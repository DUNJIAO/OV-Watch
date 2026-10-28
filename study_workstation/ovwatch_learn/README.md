# OV-Watch 学习工程

在 PC 上复刻 OV-Watch 的软件部分。

**目录结构照抄参考工程**，这样每个文件都能和 `Software/OV_Watch/` 里的原版一一对上，
不会出现"学完不知道在实机工程里对应哪个文件"的情况。

## 目录结构

```
ovwatch_learn/
├── main.c                    SDL 仿真入口（已提供，不用改）
├── Makefile                  编译脚本
│
├── User/                     ← 我们自己写的全部应用代码
│   ├── GUI_App/
│   │   ├── ui.c / ui.h       界面入口（接线代码）
│   │   └── Screens/
│   │       ├── Inc/          页面头文件
│   │       └── Src/          页面实现（主要练习区）
│   ├── Func/
│   │   ├── Inc/              应用框架层头文件
│   │   └── Src/              PageManager、pubsub、HWDataAccess、计算器……
│   └── Tasks/                FreeRTOS 任务（阶段 6 再建）
│
├── BSP/                      ← 硬件驱动层（真机代码，Keil 编译；PC 的 Makefile 不编译它）
├── Middlewares/
│   └── LVGL/                 LVGL 第三方库（直接编译参考工程里的那份）
│
└── build/  bin/              编译产物（已 gitignore）
```

## 与参考工程的对应关系

| 学习工程 | 参考工程（实机） | 说明 |
|----------|------------------|------|
| `User/GUI_App/` | `Software/OV_Watch/User/GUI_App/` | 19 个页面的代码 |
| `User/Func/` | `Software/OV_Watch/User/Func/` | 页面栈、事件总线、硬件抽象 |
| `User/Tasks/` | `Software/OV_Watch/User/Tasks/` | 13 个 FreeRTOS 任务 |
| `BSP/` | `Software/OV_Watch/BSP/` | LCD / 按键 / 电源 / 传感器驱动 |
| `Middlewares/LVGL/` | `Software/OV_Watch/Middlewares/LVGL/` | LVGL 库本身 |
| `main.c` | `lv_sim_vscode_win/main.c` | 只有 PC 仿真才有：SDL 窗口入口 |

参考工程的 PC 版本 `lv_sim_vscode_win/user_test/` 和 `Software/OV_Watch/User/` 内容几乎一致，
区别只是前者用仿真数据、后者接了真硬件。

## 谁调用谁

```
main.c  ──>  User/GUI_App/ui.c  ──>  Screens/Src/ui_*.c    （界面）
                                          │
                                          v
                                    User/Func/             （框架：页面栈、事件、硬件抽象）
                                          │
                                          v
                                      BSP/                 （驱动：读写芯片）
```

**规则：只能往下调用，不能往上。** 页面不认识 BSP，BSP 也不知道页面的存在。

## 编译运行

```bash
mingw32-make -j4          # 编译
./bin/ovwatch_learn.exe   # 运行，弹出 240x280 的仿真窗口
mingw32-make clean        # 清理
```

第一次编译要编译整个 LVGL，约 1~2 分钟；之后只重编你改过的文件。

> 重新编译前要先关掉正在运行的 exe，否则链接器会报 `Permission denied`。

## 课程

路线图和每节课的讲解都在 `../course/`。
