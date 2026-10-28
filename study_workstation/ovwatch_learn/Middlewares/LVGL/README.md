# Middlewares/LVGL

这个目录**不放 LVGL 源码**。

LVGL 8.2 的源码、配置和 SDL 驱动都在参考工程 `lv_sim_vscode_win/` 里，
Makefile 直接编译那一份：

```
lv_sim_vscode_win/
├── lvgl/            LVGL 8.2 本体（约 1000 个 .c）
├── lv_drivers/      SDL 显示 / 鼠标 / 键盘驱动
├── lv_conf.h        LVGL 配置（字体、内存、控件开关都在这里）
└── lv_drv_conf.h    驱动配置（SDL 分辨率、缩放比例）
```

这么做的原因：

1. **不复制 100MB 的第三方库**，仓库里只留一份。
2. 两边用**完全相同**的 LVGL 版本和配置，学到的行为在实机上也成立。
3. 参考工程的 PC 仿真也是这么用的。

## 想看 LVGL 源码时

| 想查什么 | 去哪 |
|----------|------|
| 某个控件怎么实现的 | `lv_sim_vscode_win/lvgl/src/widgets/` |
| 定时器怎么调度 | `lv_sim_vscode_win/lvgl/src/misc/lv_timer.c` |
| 刷新流程 | `lv_sim_vscode_win/lvgl/src/core/lv_refr.c` |
| 哪些功能被裁剪了 | `lv_sim_vscode_win/lv_conf.h` |
| 屏幕尺寸 / 缩放 | `lv_sim_vscode_win/lv_drv_conf.h` |

实机工程对应的是 `Software/OV_Watch/Middlewares/LVGL/GUI/`，版本同样是 8.2，
只是 `lv_conf.h` 里的屏幕尺寸、内存池大小和字体配置不同。

## FreeRTOS

实机还有 `Middlewares/Third_Party/FreeRTOS/`。PC 上没有这个目录——
阶段 6 用线程模拟任务时再说。
