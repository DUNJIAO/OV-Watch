# simui.ps1 —— 操作 SDL 仿真窗口的小工具（点击 / 拖拽 / 截图）
#
# 用法：
#   . .\study_workstation\tools\simui.ps1
#   $w = Start-Sim
#   Capture-Sim $w "$env:TEMP\home.png"
#   Drag-Sim    $w 50 150 210 150        # 右滑：主页 -> 菜单
#   Click-Sim   $w 120 226               # 点 BACK
#   Stop-Sim
#
# 三条踩过的坑（改这个文件前先看一眼）：
#   1. 窗口启动在 (-32000,-32000)，必须先 ShowWindow(9) + ShowWindow(5)。
#   2. SetCursorPos 不产生 SDL_MOUSEMOTION；相对 mouse_event 又被指针加速放大。
#      所以拖拽过程用 SendMessageTimeout(WM_MOUSEMOVE) 直接喂坐标。
#   3. LVGL 的 indev_gesture() 只要有一帧位移 < gesture_min_velocity(3)
#      就把 gesture_sum 清零。步长必须小、间隔必须密（默认 4 px / 12 ms）。
#   4. PrintWindow 截图滞后约 1 s，交互后先等 1.2 s。

if (-not ("SimUi" -as [type])) {
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public class SimUi {
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
    [DllImport("user32.dll")] public static extern int GetWindowThreadProcessId(IntPtr h, out int pid);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, IntPtr extra);
    [DllImport("user32.dll", SetLastError = true)] public static extern IntPtr SendMessageTimeout(IntPtr h, uint msg, IntPtr w, IntPtr l, uint flags, uint timeout, out IntPtr result);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int left, top, right, bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int x, y; }

    public static IntPtr FindByPid(int target) {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr h, IntPtr l) {
            int pid; GetWindowThreadProcessId(h, out pid);
            if (pid != target) return true;
            RECT cr; GetClientRect(h, out cr);
            if (cr.right - cr.left >= 200 && cr.bottom - cr.top >= 200) { found = h; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    public static void Capture(IntPtr h, string path) {
        RECT wr; GetWindowRect(h, out wr);
        RECT cr; GetClientRect(h, out cr);
        using (Bitmap bmp = new Bitmap(wr.right - wr.left, wr.bottom - wr.top))
        using (Graphics g = Graphics.FromImage(bmp)) {
            IntPtr hdc = g.GetHdc(); PrintWindow(h, hdc, 2); g.ReleaseHdc(hdc);
            POINT p = new POINT(); ClientToScreen(h, ref p);
            Rectangle crop = new Rectangle(p.x - wr.left, p.y - wr.top, cr.right - cr.left, cr.bottom - cr.top);
            using (Bitmap outb = bmp.Clone(crop, bmp.PixelFormat)) outb.Save(path, ImageFormat.Png);
        }
    }
    public static void MoveTo(IntPtr h, int x, int y) { POINT p = new POINT(); ClientToScreen(h, ref p); SetCursorPos(p.x + x, p.y + y); }
    public static void Click(IntPtr h, int x, int y) {
        MoveTo(h, x, y); System.Threading.Thread.Sleep(250);
        mouse_event(0x0002, 0, 0, 0, IntPtr.Zero); System.Threading.Thread.Sleep(150);
        mouse_event(0x0004, 0, 0, 0, IntPtr.Zero); System.Threading.Thread.Sleep(300);
    }
    public static void Drag(IntPtr h, int x1, int y1, int x2, int y2, int steps, int stepMs) {
        MoveTo(h, x1, y1); System.Threading.Thread.Sleep(300);
        mouse_event(0x0002, 0, 0, 0, IntPtr.Zero); System.Threading.Thread.Sleep(200);
        for (int i = 1; i <= steps; i++) {
            int tx = x1 + (x2 - x1) * i / steps, ty = y1 + (y2 - y1) * i / steps;
            MoveTo(h, tx, ty);
            IntPtr res;
            SendMessageTimeout(h, 0x0200, (IntPtr)0x0001, (IntPtr)((ty << 16) | (tx & 0xFFFF)), 0x0002, 1500, out res);
            System.Threading.Thread.Sleep(stepMs);
        }
        System.Threading.Thread.Sleep(250);
        mouse_event(0x0004, 0, 0, 0, IntPtr.Zero);
    }
}
'@
}

$script:SimExe = Join-Path $PSScriptRoot "..\ovwatch_learn\bin\ovwatch_learn.exe"
$script:SimLog = Join-Path $env:TEMP "ovwatch_sim_stdout.txt"
$script:SimProc = $null
$script:SimWin = [IntPtr]::Zero

function Start-Sim {
    param([string]$Exe = $script:SimExe, [int]$WaitMs = 2000)
    Stop-Sim
    $script:SimProc = Start-Process -FilePath $Exe -RedirectStandardOutput $script:SimLog -PassThru
    Start-Sleep -Milliseconds $WaitMs
    $script:SimWin = [SimUi]::FindByPid($script:SimProc.Id)
    if ($script:SimWin -eq [IntPtr]::Zero) { throw "窗口没找到：$Exe" }
    [SimUi]::ShowWindow($script:SimWin, 9) | Out-Null   # SW_RESTORE
    [SimUi]::ShowWindow($script:SimWin, 5) | Out-Null   # SW_SHOW
    [SimUi]::SetWindowPos($script:SimWin, [IntPtr](-1), 0, 0, 0, 0, 0x0001 -bor 0x0002) | Out-Null
    [SimUi]::SetForegroundWindow($script:SimWin) | Out-Null
    Start-Sleep -Milliseconds 1500
    return $script:SimWin
}

function Stop-Sim {
    if ($script:SimProc) { Stop-Process -Id $script:SimProc.Id -Force -ErrorAction SilentlyContinue }
    # 清掉可能残留的旧进程：它们会抢走鼠标事件
    Get-Process ovwatch_learn -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 800
    $script:SimProc = $null
    $script:SimWin = [IntPtr]::Zero
}

function Capture-Sim {
    param([IntPtr]$Win = $script:SimWin, [Parameter(Mandatory)][string]$Path)
    [SimUi]::Capture($Win, $Path)
    Write-Output "saved $Path"
}

function Click-Sim {
    param([IntPtr]$Win = $script:SimWin, [Parameter(Mandatory)][int]$X, [Parameter(Mandatory)][int]$Y)
    [SimUi]::Click($Win, $X, $Y)
}

function Drag-Sim {
    param([IntPtr]$Win = $script:SimWin,
          [Parameter(Mandatory)][int]$X1, [Parameter(Mandatory)][int]$Y1,
          [Parameter(Mandatory)][int]$X2, [Parameter(Mandatory)][int]$Y2,
          [int]$Steps = 40, [int]$StepMs = 12)
    [SimUi]::Drag($Win, $X1, $Y1, $X2, $Y2, $Steps, $StepMs)
}

function Show-SimLog {
    param([int]$Tail = 20)
    if (Test-Path $script:SimLog) { Get-Content $script:SimLog -Encoding UTF8 | Select-Object -Last $Tail }
}
