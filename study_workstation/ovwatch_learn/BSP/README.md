# BSP —— 板级支持包（Board Support Package）

**这里放"直接和芯片/外设打交道"的代码。** 只干一件事：把寄存器/总线操作封成函数，
上层（`User/Func/`）调用它，它不主动调上层。

## 重要：BSP 是真机代码，PC 不编译

`BSP/` 下的文件用的是 STM32 HAL（`stm32f4xx_hal.h`、`HAL_GPIO_WritePin`、SPI/I2C 句柄……），
PC 上根本编不过。所以：

| 谁编译 `BSP/` | 什么时候 |
|---------------|----------|
| Keil 工程（真机） | 烧进手表时 |
| PC 的 Makefile | **不编译**（`SRC` 里没有 `BSP`） |

PC 上要"有数据可显示"，走的是另一条路：`User/Func/HWDataAccess.c` 里的
`#if HW_USE_HARDWARE ... #else ...` 分支。**仿真数据写在 `User/Func/` 里，不放进 BSP。**
（参考工程的 PC 仿真也是这么做的——`lv_sim_vscode_win/user_test/` 里一份 BSP 都没有。）

## 参考工程里的 13 个驱动

```
Software/OV_Watch/BSP/
├── AHT21/        温湿度传感器（I2C）
├── BL24C02/      EEPROM（存设置），附带 DataSave.c
├── EM7028/       心率 / 血氧（I2C）
├── IIC/          软件 I2C 总线（上面几个传感器的地基）
├── KEY/          按键（GPIO 扫描）
├── KT6328/       蓝牙模块（串口）
├── LCD/          ST7789 屏幕（SPI）+ 背光 PWM
├── LSM303DLH/    电子罗盘
├── MPU6050/      六轴（计步、抬腕），带 eMPL 移植
├── OWDG/         独立看门狗
├── POWER/        电源管理 / 电量（ADC + 充电检测 + 关机）
├── SPL06_001/    气压计
└── TOUCH/        触摸屏 CST816
```

我们会写的子集和顺序见 [阶段 7](../course/README.md)：LCD → KEY → POWER → RTC →
MPU6050 → AHT21 → EM7028 → BL24C02。

## 写代码时记住

- **函数名和参考的 BSP 完全一致**（`LCD_Init()`、`PowerCalculate()`、`AHT_Read()`……）。
  `HWDataAccess.c` 的 `#if HW_USE_HARDWARE` 分支是按这些名字写死的，改名就接不上。
- 不要 `#include` 任何 LVGL 头文件 —— BSP 不知道自己画在哪儿，只知道往显存里写字节。
- 不要 `#include` 任何 `User/` 下的头文件 —— 依赖只能从上往下，不能从下往上。
- 每个驱动配一个能在串口/屏幕上"看见"的验证动作（屏刷新、串口打印），
  否则写完了也不知道对不对。

## 我们的 BSP 怎么进 Keil 工程

不进 `Software/OV_Watch/MDK-ARM` 那个工程（那是老师，不能改）。

做法是复制一份自己的工程到 `study_workstation/hw_learn/`，把 `User/` 和 `BSP/` 换成
学习工程里的这两份。这样：

- 参考工程保持原样，随时可以对照
- 我们写的驱动和参考的驱动**同名**，一个模块一个模块地替换过去
- 面板（Screens）继续在 PC 上开发和截图对比，代码原样搬进 Keil 编译

> 这一步在单元 7.1 动手时落地，现在 `BSP/` 是空的。
