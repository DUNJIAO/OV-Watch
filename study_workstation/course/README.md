# OV-Watch 软件逐块还原

原则：**一次只还原一个模块，每个模块都能单独编译、运行、和参考画面对比。**

- 你写的代码：`study_workstation/ovwatch_learn/User/`（目录结构和参考工程的 `User/` 一致）
- 硬件驱动：`study_workstation/ovwatch_learn/BSP/`（真机代码，PC 轨的 Makefile 不编译它；仿真数据在 `User/Func/HWDataAccess.c` 的 `#else` 分支里）
- 参考实现：`Software/OV_Watch/User/`（实机）与 `lv_sim_vscode_win/user_test/`（PC 版）
- 每块完成后我 review，再进入下一块
- 先完成「核心闭环」，游戏、NFC、指南针等可以最后补
- 每课收尾跑一次 `study_workstation/tools/ref_diff.sh`，和参考工程的差异记在 [对照表](对照表.md)
- 想在仿真器里自动点一点、滑一滑、截图：`study_workstation/tools/simui.ps1`

图例：`[ ]` 未开始　`[~]` 进行中　`[x]` 已完成

---

## 阶段 0：地基（已由我搭好）

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 0.1 | 仿真工程、Makefile、SDL 入口 | `main.c`、`Makefile` | `lv_sim_vscode_win/main.c` | `[x]` |
| 0.2 | 界面入口：主题 + `ui_init` | `User/GUI_App/ui.c`、`ui.h` | `user_test/GUI_App/ui.c` | `[x]` |
| 0.3 | 字体资源（Cuyuan / iconfont） | Makefile 直接编译参考字体 | `user_test/GUI_App/Fonts/` | `[x]` |

> 0.3 没有复制字体文件，而是在 Makefile 里直接编译参考工程的字体源码。

---

## 阶段 1：主页闭环

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 1.1 | 主页静态布局：时间、日期、电量、步数、温湿度、心率 | `Screens/Src/ui_HomePage.c` | `ui_HomePage.c` | `[x]` |
| 1.2 | 主页动态刷新：`lv_timer` + 假数据 | `ui_HomePage.c` | 同文件 `HomePage_timer_cb` | `[x]` |
| 1.3 | 硬件抽象层 `HWDataAccess`（先接仿真数据） | `User/Func/Src/HWDataAccess.c` | `Func/Src/HWDataAccess.c` | `[x]` |
| 1.4 | 下拉面板：NFC / BLE / 电源 / 设置 / 亮度 | `ui_HomePage.c` | 同文件 543 行起 | `[x]` |

**阶段验收**：主页显示真实系统时间、电量、步数，下拉面板能开合。

---

## 阶段 2：页面导航框架

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 2.1 | 页面栈 `Page_t` + push/pop/load/back | `User/Func/Src/PageManager.c` | `Func/Src/PageManager.c` | `[x]` |
| 2.2 | 发布/订阅事件总线 | `User/Func/Src/pubsub.c` | `Func/Src/pubsub.c` | `[ ]` |
| 2.3 | 菜单页：滚动列表 + 图标 | `Screens/Src/ui_MenuPage.c` | `ui_MenuPage.c` | `[ ]` |
| 2.4 | PC 按键 → 返回上一页 | `User/GUI_App/ui.c` 回调 | `user_test/GUI_App/ui.c` | `[ ]` |
| 2.5 | 页面切换动画 | `PageManager.c` | `PageManager.c` | `[ ]` |

**阶段验收**：主页 ↔ 菜单可来回切换，按退格键能返回，返回后页面状态正确。

> 2.2 做完要删掉 `User/Func/Src/pubsub_stub.c`。

---

## 阶段 3：设置与持久化

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 3.1 | 设置页 `SetPage` | `ui_SetPage.c` | `ui_SetPage.c` | `[ ]` |
| 3.2 | 日期时间设置 | `ui_DateTimeSetPage.c` | 同文件 | `[ ]` |
| 3.3 | 熄屏时间设置 | `ui_OffTimePage.c` | 同文件 | `[ ]` |
| 3.4 | 数据存储（EEPROM 用文件模拟） | `User/Func/Src/DataSave.c` | `BSP/BL24C02/DataSave.c` | `[ ]` |
| 3.5 | 关于页 | `ui_AboutPage.c` | 同文件 | `[ ]` |

**阶段验收**：改完设置，关闭程序再打开，设置还在。

---

## 阶段 4：传感器与工具页

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 4.1 | 心率页 + 峰值检测算法 | `ui_HRPage.c`、`HrAlgorythm.c` | 同文件 | `[ ]` |
| 4.2 | 血氧页 | `ui_SPO2Page.c` | 同文件 | `[ ]` |
| 4.3 | 环境页（温湿度） | `ui_EnvPage.c` | 同文件 | `[ ]` |
| 4.4 | 秒表 / 倒计时 | `ui_TimerPage.c` | 同文件 | `[ ]` |
| 4.5 | 日历页 | `ui_CalendarPage.c` | 同文件 | `[ ]` |
| 4.6 | 计算器（双栈求值） | `ui_ComputerPage.c`、`StrCalculate.c` | `Func/Src/StrCalculate.c` | `[ ]` |
| 4.7 | 指南针（可选） | `ui_CompassPage.c` | 同文件 | `[ ]` |
| 4.8 | NFC 卡包（可选） | `ui_NFCCardPage.c` | 同文件 | `[ ]` |

---

## 阶段 5：游戏与系统页

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 5.1 | 游戏选择页 | `ui_GameSelectPage.c` | 同文件 | `[ ]` |
| 5.2 | 2048 | `ui_Game2048Page.c` | 同文件 | `[ ]` |
| 5.3 | 记忆翻牌 | `ui_GameMemPage.c` | 同文件 | `[ ]` |
| 5.4 | 充电动画页 | `ui_ChargPage.c` | 同文件 | `[ ]` |
| 5.5 | 弹窗组件 | `ui_Megboxes.c` | 同文件 | `[ ]` |
| 5.6 | 关机流程 | `ui_HomePage.c` 电源部分 | 同文件 | `[ ]` |

---

## 阶段 6：系统整合（仍在 PC）

| 单元 | 内容 | 你的文件 | 参考文件 | 状态 |
|------|------|----------|----------|------|
| 6.1 | 用线程模拟 FreeRTOS 任务 | `User/Tasks/` | `User/Tasks/Src/` | `[ ]` |
| 6.2 | 抬手唤醒 / 空闲熄屏状态机 | `User/Tasks/` | `user_RunModeTasks.c` | `[ ]` |
| 6.3 | 充电检测、消息发送 | `User/Tasks/` | 对应任务文件 | `[ ]` |

**阶段验收**：PC 版行为与参考仿真器基本一致。

---

## 阶段 7：硬件轨（有实机就能做，可以和第 1–6 阶段并行）

有实物手表时，**BSP 不需要仿真**：直接写真实驱动、烧进手表看效果。
这条路和 PC 轨共用同一份 `User/` 代码，区别只在 `HW_USE_HARDWARE` 这个开关。

详细说明见 [硬件轨：在真机上写 BSP](硬件轨.md)：环境核实结果、Flash 分区与烧录注意事项、
bring-up 清单、BSP 编写规范。

| 单元 | 内容 | 怎么验证 | 参考文件 |
|------|------|----------|----------|
| 7.0 | **上电 bring-up**：编译并烧录参考固件，确认工具链、屏幕、串口 | 屏幕亮起来 | `MDK-ARM/OV_Watch.uvprojx` |
| 7.1 | LCD + 背光 | 刷纯色、画一行字 | `BSP/LCD/` |
| 7.2 | KEY 按键 | 按下后串口打印 / 屏幕反应 | `BSP/KEY/` |
| 7.3 | POWER：电量、充电检测 | 串口打印电压和百分比 | `BSP/POWER/` |
| 7.4 | RTC | 屏上显示真实时间 | `Core/Src/rtc.c` |
| 7.5 | MPU6050：计步、抬腕 | 摇手表看步数涨 | `BSP/MPU6050/` |
| 7.6 | AHT21：温湿度 | 哈一口气看数值 | `BSP/AHT21/` |
| 7.7 | EM7028：心率 / 血氧 | 手指按住传感器 | `BSP/EM7028/` |
| 7.8 | BL24C02：EEPROM | 存设置、断电再上电 | `BSP/BL24C02/` |
| 7.9 | FreeRTOS 任务层 + STOP 低功耗 | 整机行为、续航 | `User/Tasks/` |

**顺序原则**：先 LCD。其他驱动的结果最终都要显示出来，屏幕不通什么都验证不了。

**每个驱动配一个最小测试**：不接 LVGL、不接任务，`main()` 里直接调驱动函数。
参考 `Software/IAP_F411/Core/Src/main.c`——那个 bootloader 没跑 LVGL，照样能在屏上写字。

### ⚠️ 烧录前必看

手表 Flash 前面 48KB（`0x08000000`–`0x0800BFFF`）是 IAP bootloader，**不要覆盖**。

- Keil 工程里 IROM1 已经是 `0x0800C000`，**别改**
- Options → Debug → Settings → Flash Download，选 **Erase Sectors**，不要选 Erase Full Chip
- 保险起见，先把当前固件备份出来（仓库里 `MDK-ARM/output/OV_Watch.hex` 就是一份）
- 也可以走 IAP（串口 Ymodem）刷机，那条路根本不碰 bootloader

---

## 阶段 8：IAP 固件升级（实机）

单独的一个工程：`Software/IAP_F411`。它住在 Flash 最前面，负责把新固件通过串口写进后面的
Flash，然后跳过去运行——**手表上的"刷机"功能**。

| 单元 | 内容 | 参考文件 | 状态 |
|------|------|----------|------|
| 8.1 | Flash 分区与向量表跳转：boot 在 `0x08000000`，app 在 `0x0800C000`，app 启动时 `SCB->VTOR = 0x0000C000` | `IAP_F411/Core/Src/main.c`、`OV_Watch/Core/Src/main.c` | `[ ]` |
| 8.2 | 启动流程：按住 KEY1 进 boot 菜单 / 校验魔术字 / 跳转或提示 No App | 同上 | `[ ]` |
| 8.3 | Ymodem 协议收发 | `IAP_F411/Ymodem/ymodem.c` | `[ ]` |
| 8.4 | Flash 擦除 / 写入 / 校验（按扇区） | `IAP_F411/Ymodem/flash_if.c` | `[ ]` |
| 8.5 | boot 菜单与 LCD 进度显示 | `IAP_F411/Ymodem/menu.c`、`IAP_F411/BSP/LCD/` | `[ ]` |
| 8.6 | 应用侧配合：升级入口、写魔术字、跳回 boot | `OV_Watch/User/` | `[ ]` |

**验收**：通过串口用 Ymodem 把新的 app 固件烧进手表，重启后跑的是新程序。

> 8.3 / 8.4 可以在 PC 上先验证：用文件模拟 Flash，把 `Flash_If_Write` 换成写文件，
> Ymodem 用管道喂数据。8.1 / 8.2 / 8.5 涉及真实 Flash 和真实屏幕，必须在实机上做。
>
> 依赖：8.5 用到阶段 7.2 的 BSP（LCD / 按键 / 电源），所以放在实机移植之后。

---

## 当前进度

**当前代码在单元 2.1：页面栈 + 占位菜单页**，讲解见 [第 5 课](05_页面栈.md)。

- 第 1 课 [主页静态布局](01_主页.md)：控件树、对齐偏移、圆弧三部分
- 第 2 课 [动态刷新](02_动态刷新.md)：`lv_timer` 的创建 / 回调 / 销毁
- 第 3 课 [硬件抽象层](03_硬件抽象层.md)：`HWInterface` 函数指针、`HW_USE_HARDWARE` 开关、仿真数据源
- 第 4 课 [下拉面板](04_下拉面板.md)：事件回调、`CHECKABLE` 状态样式、滚动当动画、手势冒泡
- 第 5 课 [页面栈](05_页面栈.md)：`Page_t` 三件套、`auto_del` 为什么逼着你重新 `init`、字体子集

**2.1 的现状**：主页和菜单页都交给 `PageStack` 管，右滑进菜单、`BACK` 回主页都通了。
菜单页是 80 行的占位版本（英文，因为现有字体没有"菜单"两个字），2.3 课换成真菜单。
`HW_USE_HARDWARE` 仍是 `0`，想上真机时改成 `1`，接上 `BSP/` 的驱动即可。

### 有实机之后的安排

现在有两条轨，可以并行推进：

| 轨 | 内容 | 文档 |
|----|------|------|
| PC 轨 | 2.2 起：页面和逻辑，改完几秒钟就能看效果 | [第 3 课](03_硬件抽象层.md) 起 |
| 硬件轨 | 阶段 7：先 bring-up，再一个模块一个模块写 BSP，烧进手表验证 | [硬件轨](硬件轨.md) |

环境已经核实过：Keil MDK 5.24a + ARMCC V5.06 在本机可以完整编译参考工程
（0 error / 197 warning / Code 202124 字节 / 22 秒，命令行可跑）。

下一单元（PC 轨）：**2.2 发布/订阅事件总线 `pubsub`** —— 页面栈管"在哪一页"，
`pubsub` 管"数据变了通知谁"。做完之后，之前为了链接临时补的 `pubsub_stub.c` 就能删掉了，
设置页改时间也不用直接去改主页的控件。
