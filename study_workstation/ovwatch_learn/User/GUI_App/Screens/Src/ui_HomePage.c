/**
 * 主页
 *
 * 对应参考实现：
 *   Software/OV_Watch/User/GUI_App/Screens/Src/ui_HomePage.c
 *
 * 1.1 课：画出静态布局
 * 1.2 课：加 lv_timer，用假时钟驱动刷新
 * 1.3 课：数据全部改成从 HWInterface 读，假时钟换成真实 RTC
 * 1.4 课：下拉面板 + 事件绑定（第一次"写"抽象层：改背光、开关蓝牙、关机）
 */
#include <stdio.h>   /* 1.4 课用 printf 在控制台打印事件，方便确认真的触发了 */

#include "../Inc/ui_HomePage.h"
#include "../../../Func/Inc/HWDataAccess.h"   /* 和参考实现同一个相对层级 */

/* ==================== 控件指针 ==================== */

/* 屏幕 */
lv_obj_t *ui_HomePage = NULL;

/* 时间 */
lv_obj_t *ui_TimeHourLabel   = NULL;
lv_obj_t *ui_TimeColonLabel  = NULL;
lv_obj_t *ui_TimeMinuteLabel = NULL;

/* 电量 */
lv_obj_t *ui_BatArc       = NULL;
lv_obj_t *ui_BaticonLabel = NULL;
lv_obj_t *ui_BatNumLabel  = NULL;

/* 日期 */
lv_obj_t *ui_DateLabel = NULL;
lv_obj_t *ui_DayLabel  = NULL;

/* 步数 */
lv_obj_t *ui_StepiconLabel = NULL;
lv_obj_t *ui_StepCnLabel   = NULL;
lv_obj_t *ui_StepNumLabel  = NULL;
lv_obj_t *ui_StepNumBar    = NULL;

/* 温度 */
lv_obj_t *ui_TempArc       = NULL;
lv_obj_t *ui_TempiconLabel = NULL;
lv_obj_t *ui_TempNumLabel  = NULL;

/* 湿度 */
lv_obj_t *ui_HumiiconLabel = NULL;
lv_obj_t *ui_HumiArc       = NULL;
lv_obj_t *ui_HumiNumLabel  = NULL;

/* 心率 */
lv_obj_t *ui_HRArc       = NULL;
lv_obj_t *ui_HRiconLabel = NULL;
lv_obj_t *ui_HRNumLabel  = NULL;

/* ==================== 下拉面板控件（1.4 课） ==================== */

lv_obj_t *ui_DropDownPanel = NULL;   /* 比屏幕还长的滚动容器 */
lv_obj_t *ui_UpBGPanel     = NULL;   /* 拉出来时看到的那条 130 高的底色 */
lv_obj_t *ui_DownBGPanel   = NULL;   /* 垫在下面，给滚动留出行程 */

lv_obj_t *ui_NFCButton   = NULL;
lv_obj_t *ui_NFCLabel    = NULL;
lv_obj_t *ui_BLEButton   = NULL;
lv_obj_t *ui_BLELabel    = NULL;
lv_obj_t *ui_PowerButton = NULL;
lv_obj_t *ui_PowerLabel  = NULL;
lv_obj_t *ui_SetButton   = NULL;
lv_obj_t *ui_SetLabel    = NULL;
lv_obj_t *ui_LightSlider = NULL;
lv_obj_t *ui_LightLabel  = NULL;

/*
 * 面板上的三个状态。它们不是硬件数据，是"开关现在长什么样"的记忆：
 * 页面重建时靠它们把按钮恢复到上次的样子（参考实现在 init 里 if(ui_HomePageNFCEN) 加 CHECKED）。
 */
uint8_t ui_HomePageNFCEN    = 0;
uint8_t ui_HomePageBLEEN    = 0;
uint8_t ui_LightSliderValue = 100;

/* ==================== 页面数据 ==================== */

/*
 * 这些变量现在只干两件事：
 *   1. 第一帧显示时当初始值用（定时器还没跑）
 *   2. 定时器回调里当「上次显示的是什么」的缓存，用来判断要不要更新
 * 真正的数据来源是 HWInterface，见下面的 HomePage_timer_cb。
 */

uint8_t ui_TimeHourValue   = 8;
uint8_t ui_TimeMinuteValue = 5;
const char *ui_Days[7] = {"Mon.", "Tue.", "Wed.", "Thu.", "Fri.", "Sat.", "Sun."};
uint8_t ui_DateMonthValue   = 11;
uint8_t ui_DateDayValue     = 8;
uint8_t ui_DataWeekdayValue = 2; /* 1=Mon ... 7=Sun */

uint8_t  ui_BatArcValue       = 70;
uint16_t ui_StepNumValue      = 0;
int8_t   ui_HomePageTempValue = 25;
int8_t   ui_HomePageHumiValue = 67;
int16_t  ui_HomePageHRValue   = 72;

/*
 * 图标字体的字形是私用区字符，直接用 UTF-8 十六进制写出来，
 * 例如 "\xEE\x9A\x8F" 就是参考代码里那个看不见的电池图标。
 */
#define ICON_BAT  "\xEE\x9A\x8F"
#define ICON_STEP "\xEE\x99\xB6"
#define ICON_TEMP "\xEE\x99\x99"
#define ICON_HUMI "\xEE\x99\xB3"
#define ICON_HR   "\xEE\x9D\xA2"
#define TEXT_STEP_CN "\xE4\xBB\x8A\xE6\x97\xA5\xE6\xAD\xA5\xE6\x95\xB0" /* 今日步数 */

/* 下拉面板上的五个图标（同样是从参考实现的源码里抠出来的 UTF-8 字节） */
#define ICON_NFC   "\xEE\xA6\x86"
#define ICON_BLE   "\xEE\x98\x87"
#define ICON_POWER "\xEE\x98\xAC"
#define ICON_SET   "\xEE\x98\x80"
#define ICON_LIGHT "\xEE\x99\xBA"

/* ==================== 动态刷新（1.2 / 1.3 课） ==================== */

static lv_timer_t *ui_HomePageTimer = NULL;

/*
 * 每 500 ms 从 HWInterface 读一次数据。
 *
 * 这里有两层"变了才更新"：
 *   1. 定时器 500 ms 跑一次，但 RTC 只有秒变化、温度几秒才变一次，
 *      大部分 tick 读完发现什么都没变，直接走完拉倒。
 *   2. 每个字段都先比较再写。值没变就不碰 label——参考工程每个字段都这么写，
 *      避免让 LVGL 白白重绘（第 2 课讲过原因）。
 *
 * 注意：参考工程里这一整段外面还包了一层
 *     if (Page_Get_NowPage()->page_obj == &ui_HomePage)
 * 也就是"主页不是当前页就别刷"。我们目前只有一页，用不上；
 * 2.1 课做多页面导航时必须补上，否则切到别的页面后这里还在改主页的控件。
 */
static void HomePage_timer_cb(lv_timer_t *timer)
{
    /* 16 而不是 12：gcc 对 "%2d-%02d" 按最坏情况算要 15 字节，
     * 实际月份/日期最多两位，12 也够，但编译器会一直报警告。 */
    char buf[16];
    HW_DateTimeTypeDef DateTime;

    (void)timer;

    /* 数据从哪来？硬件抽象层说了算：实机读 RTC 芯片，PC 读 localtime */
    HWInterface.RealTimeClock.GetTimeDate(&DateTime);

    if (ui_TimeMinuteValue != DateTime.Minutes) {
        ui_TimeMinuteValue = DateTime.Minutes;
        sprintf(buf, "%02d", ui_TimeMinuteValue);
        lv_label_set_text(ui_TimeMinuteLabel, buf);
    }

    if (ui_TimeHourValue != DateTime.Hours) {
        ui_TimeHourValue = DateTime.Hours;
        sprintf(buf, "%2d", ui_TimeHourValue);
        lv_label_set_text(ui_TimeHourLabel, buf);
    }

    /* 跨天时日期和星期一起更新 */
    if (ui_DateDayValue != DateTime.Date) {
        ui_DateDayValue     = DateTime.Date;
        ui_DataWeekdayValue = DateTime.WeekDay;
        sprintf(buf, "%2d-%02d", ui_DateMonthValue, ui_DateDayValue);
        lv_label_set_text(ui_DateLabel, buf);
        lv_label_set_text(ui_DayLabel, ui_Days[ui_DataWeekdayValue - 1]);
    }

    /* 跨月 */
    if (ui_DateMonthValue != DateTime.Month) {
        ui_DateMonthValue   = DateTime.Month;
        ui_DateDayValue     = DateTime.Date;
        ui_DataWeekdayValue = DateTime.WeekDay;
        sprintf(buf, "%2d-%02d", ui_DateMonthValue, ui_DateDayValue);
        lv_label_set_text(ui_DateLabel, buf);
        lv_label_set_text(ui_DayLabel, ui_Days[ui_DataWeekdayValue - 1]);
    }

    if (ui_BatArcValue != HWInterface.Power.power_remain) {
        ui_BatArcValue = HWInterface.Power.power_remain;
        lv_arc_set_value(ui_BatArc, ui_BatArcValue);
        sprintf(buf, "%2d%%", ui_BatArcValue);
        lv_label_set_text(ui_BatNumLabel, buf);
    }

    if (ui_StepNumValue != HWInterface.IMU.Steps) {
        ui_StepNumValue = HWInterface.IMU.Steps;
        sprintf(buf, "%d", ui_StepNumValue);
        lv_label_set_text(ui_StepNumLabel, buf);
    }

    /* AHT21 的字段是 uint8_t，用的时候按 int8_t 解释（参考工程也这么写） */
    if (ui_HomePageTempValue != (int8_t)HWInterface.AHT21.temperature) {
        ui_HomePageTempValue = (int8_t)HWInterface.AHT21.temperature;
        lv_arc_set_value(ui_TempArc, ui_HomePageTempValue);
        sprintf(buf, "%d", ui_HomePageTempValue);
        lv_label_set_text(ui_TempNumLabel, buf);
    }

    if (ui_HomePageHumiValue != (int8_t)HWInterface.AHT21.humidity) {
        ui_HomePageHumiValue = (int8_t)HWInterface.AHT21.humidity;
        lv_arc_set_value(ui_HumiArc, ui_HomePageHumiValue);
        sprintf(buf, "%d", ui_HomePageHumiValue);
        lv_label_set_text(ui_HumiNumLabel, buf);
    }

    /* 参考工程主页不看心率（心率页面单独刷新）。我们顺手刷一下，
     * 让主页的心率圈也会动，便于观察抽象层确实在出数据。 */
    if (ui_HomePageHRValue != HWInterface.HR_meter.HrRate) {
        ui_HomePageHRValue = HWInterface.HR_meter.HrRate;
        lv_arc_set_value(ui_HRArc, ui_HomePageHRValue);
        sprintf(buf, "%d", ui_HomePageHRValue);
        lv_label_set_text(ui_HRNumLabel, buf);
    }
}

/* ==================== 事件回调（1.4 课） ==================== */

/*
 * 事件和定时器是两套完全不同的东西，别混：
 *
 *   定时器：时间到了就响，没人操作也响（1.2 课）
 *   事件  ：有人干了什么才响——点一下、按住、松开、滑一下、值变了
 *
 * 注册方式：
 *   lv_obj_add_event_cb(哪个控件, 哪个函数, 监听哪些事件, 附带数据)
 * 第三个参数写 LV_EVENT_ALL 表示"全都报给我"，然后在函数里按 code 分流。
 * 只关心一种事件时也可以只写那一种（比如 LV_EVENT_CLICKED）。
 */

/* 屏幕级事件：主页收到的是手势（往右滑）。参考实现在这里 Page_Load(&Page_Menu) */
static void ui_event_HomePage(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }
    if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
        printf("[1.4] 收到右滑手势（参考实现跳菜单页，等 2.1 课有 PageManager 再接）\n");
    }
}

/* NFC 开关：参考实现在这里 ICcard_Select()。我们的还原范围里没有 NFC 芯片 */
static void ui_event_NFCButton(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    /* 按钮加了 LV_OBJ_FLAG_CHECKABLE，点一下就在"选中 / 未选中"之间翻 */
    ui_HomePageNFCEN = lv_obj_has_state(target, LV_STATE_CHECKED) ? 1 : 0;
}

/* 蓝牙开关：第一次"写"抽象层 —— 界面说"我要蓝牙开"，怎么开是驱动的事 */
static void ui_event_BLEButton(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (lv_obj_has_state(target, LV_STATE_CHECKED)) {
        HWInterface.BLE.Enable();
        ui_HomePageBLEEN = 1;
    } else {
        HWInterface.BLE.Disable();
        ui_HomePageBLEEN = 0;
    }
}

/* 电源：参考实现是跳到"滑动关机"页面，我们还没有页面栈，直接演示关机 */
static void ui_event_PowerButton(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    HWInterface.Power.Shutdown();
}

/* 设置：参考实现 Page_Load(&Page_Set)，2.1 课做完页面栈再接 */
static void ui_event_SetButton(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    printf("[1.4] 设置按钮被点击（等 2.1 课有 PageManager 再跳转）\n");
}

/* 亮度滑块：拖一下就调一次背光 */
static void ui_event_LightSlider(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    ui_LightSliderValue = (uint8_t)lv_slider_get_value(ui_LightSlider);
    HWInterface.LCD.SetLight(ui_LightSliderValue);   /* 实机上改的是背光 PWM 占空比 */
}

/* ==================== 下拉面板（1.4 课） ==================== */

/*
 * 面板不是"点一下弹出来"的菜单，而是一个比屏幕还长的滚动容器：
 *
 *   ┌──────────── 屏幕 280 高 ────────────┐
 *   │ 面板从 y=-10 开始，高 420           │
 *   │ ┌───────────────────────────────┐   │
 *   │ │ UpBGPanel 240×130（这行才是面板）│   │
 *   │ │  NFC    电源        亮度 ▂▂    │   │
 *   │ │  BLE    设置                  │   │
 *   │ └───────────────────────────────┘   │
 *   │   …面板剩下的 290 高是空的…         │  ← 留给滚动的行程
 *   └─────────────────────────────────────┘
 *
 * 全部建完之后 lv_obj_scroll_by(0, -130) 把内容往上顶 130，
 * 按钮全部滑出屏幕顶边，看起来就是"收起来了"。
 * 用户按住空白的下沿往上/往下拖，LVGL 的滚动机制把内容拖回来，面板就被"拉"出来。
 * 一行动画代码都没写，全靠滚动。
 *
 * 注意：这句必须写在所有子控件建好之后。scroll_by 的做法是
 * 「把容器里现有的子控件整体挪一下」，容器还是空的时候调用，位移就丢了。
 * 参考工程把这句写在创建容器之后、放子控件之前；我们也照那个顺序试过，
 * PC 上面板一启动就是展开的。
 */
static void HomePage_create_dropdown_panel(void)
{
    /* ---------- 长容器 ---------- */
    ui_DropDownPanel = lv_obj_create(ui_HomePage);
    lv_obj_set_size(ui_DropDownPanel, 240, 420);
    lv_obj_align(ui_DropDownPanel, LV_ALIGN_TOP_MID, 0, -10);
    lv_obj_clear_flag(ui_DropDownPanel, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_scroll_dir(ui_DropDownPanel, LV_DIR_VER);
    lv_obj_set_style_bg_opa(ui_DropDownPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_DropDownPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DropDownPanel, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DropDownPanel, 0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

    /* ---------- 面板底色：半透明深灰，盖住下面的主页 ---------- */
    ui_UpBGPanel = lv_obj_create(ui_DropDownPanel);
    lv_obj_set_size(ui_UpBGPanel, 240, 130);
    lv_obj_align(ui_UpBGPanel, LV_ALIGN_TOP_MID, 0, -10);
    lv_obj_clear_flag(ui_UpBGPanel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_UpBGPanel, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_UpBGPanel, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- NFC（左列上） ---------- */
    ui_NFCButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_size(ui_NFCButton, 50, 50);
    lv_obj_align(ui_NFCButton, LV_ALIGN_TOP_LEFT, 0, 5);
    lv_obj_add_flag(ui_NFCButton, LV_OBJ_FLAG_CHECKABLE);   /* ← 关键：能"选中/取消" */
    lv_obj_clear_flag(ui_NFCButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_NFCButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_NFCButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_NFCButton, lv_color_hex(0x3264C8), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_NFCButton, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    if (ui_HomePageNFCEN) {
        lv_obj_add_state(ui_NFCButton, LV_STATE_CHECKED);   /* 页面重建时恢复上次状态 */
    }

    ui_NFCLabel = lv_label_create(ui_NFCButton);
    lv_obj_set_align(ui_NFCLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_NFCLabel, ICON_NFC);
    lv_obj_set_style_text_color(ui_NFCLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NFCLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- BLE（左列下） ---------- */
    ui_BLEButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_size(ui_BLEButton, 50, 50);
    lv_obj_align(ui_BLEButton, LV_ALIGN_TOP_LEFT, 0, 65);
    lv_obj_add_flag(ui_BLEButton, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_clear_flag(ui_BLEButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_BLEButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BLEButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BLEButton, lv_color_hex(0x3264C8), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_BLEButton, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    if (ui_HomePageBLEEN) {
        lv_obj_add_state(ui_BLEButton, LV_STATE_CHECKED);
    }

    ui_BLELabel = lv_label_create(ui_BLEButton);
    lv_obj_set_align(ui_BLELabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_BLELabel, ICON_BLE);
    lv_obj_set_style_text_color(ui_BLELabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BLELabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 电源（中列上） ---------- */
    ui_PowerButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_size(ui_PowerButton, 50, 50);
    lv_obj_align(ui_PowerButton, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_clear_flag(ui_PowerButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_PowerButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PowerButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PowerLabel = lv_label_create(ui_PowerButton);
    lv_obj_set_align(ui_PowerLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_PowerLabel, ICON_POWER);
    lv_obj_set_style_text_color(ui_PowerLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_PowerLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 设置（中列下） ---------- */
    ui_SetButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_size(ui_SetButton, 50, 50);
    lv_obj_align(ui_SetButton, LV_ALIGN_TOP_MID, 0, 65);
    lv_obj_clear_flag(ui_SetButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_SetButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SetButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SetLabel = lv_label_create(ui_SetButton);
    lv_obj_set_align(ui_SetLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SetLabel, ICON_SET);
    lv_obj_set_style_text_color(ui_SetLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SetLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 亮度滑块（右列） ---------- */
    ui_LightSlider = lv_slider_create(ui_DropDownPanel);
    lv_obj_set_size(ui_LightSlider, 50, 110);
    lv_obj_align(ui_LightSlider, LV_ALIGN_TOP_RIGHT, 0, 5);
    lv_slider_set_value(ui_LightSlider, ui_LightSliderValue, LV_ANIM_OFF);
    lv_obj_set_style_radius(ui_LightSlider, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightSlider, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightSlider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_LightSlider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightSlider, lv_color_hex(0x3264C8), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightSlider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightSlider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_LightLabel = lv_label_create(ui_LightSlider);
    lv_obj_set_align(ui_LightLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LightLabel, ICON_LIGHT);
    lv_obj_set_style_text_color(ui_LightLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LightLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 底部垫片：透明，只为给滚动留出行程 ---------- */
    ui_DownBGPanel = lv_obj_create(ui_DropDownPanel);
    lv_obj_set_size(ui_DownBGPanel, 240, 130);
    lv_obj_align(ui_DownBGPanel, LV_ALIGN_TOP_MID, 0, 420);
    lv_obj_clear_flag(ui_DownBGPanel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui_DownBGPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_DownBGPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 初始状态：收起 ----------
     * 放在最后，是因为 lv_obj_scroll_by 的实质是「把现有子控件整体挪一下」，
     * 容器还是空的时候调用，这一下就白挪了。 */
    lv_obj_scroll_by(ui_DropDownPanel, 0, -130, LV_ANIM_OFF);

    /* ---------- 把回调和控件接起来 ---------- */
    lv_obj_add_event_cb(ui_HomePage, ui_event_HomePage, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_NFCButton, ui_event_NFCButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_BLEButton, ui_event_BLEButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_PowerButton, ui_event_PowerButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SetButton, ui_event_SetButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_LightSlider, ui_event_LightSlider, LV_EVENT_ALL, NULL);
}

/* ==================== 创建主页 ==================== */

void ui_HomePage_screen_init(void)
{
    char value_strbuf[12];

    /* 屏幕本身也是一个对象，NULL 表示新建一块屏幕 */
    ui_HomePage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_HomePage, LV_OBJ_FLAG_SCROLLABLE);

    /* ---------- 时间：11 : 59 ---------- */

    ui_TimeHourLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_TimeHourLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_TimeHourLabel, -105);
    lv_obj_set_y(ui_TimeHourLabel, -70);
    lv_obj_set_align(ui_TimeHourLabel, LV_ALIGN_RIGHT_MID);
    sprintf(value_strbuf, "%2d", ui_TimeHourValue);
    lv_label_set_text(ui_TimeHourLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_TimeHourLabel, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimeColonLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_TimeColonLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_TimeColonLabel, -82);
    lv_obj_set_y(ui_TimeColonLabel, -70);
    lv_obj_set_align(ui_TimeColonLabel, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_TimeColonLabel, ":");
    lv_obj_set_style_text_color(ui_TimeColonLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TimeColonLabel, &ui_font_Cuyuan48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimeMinuteLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_TimeMinuteLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_TimeMinuteLabel, -20);
    lv_obj_set_y(ui_TimeMinuteLabel, -70);
    lv_obj_set_align(ui_TimeMinuteLabel, LV_ALIGN_RIGHT_MID);
    sprintf(value_strbuf, "%02d", ui_TimeMinuteValue);
    lv_label_set_text(ui_TimeMinuteLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_TimeMinuteLabel, &ui_font_Cuyuan48, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 电量：左上角圆弧 + 图标 + 百分比 ---------- */

    ui_BatArc = lv_arc_create(ui_HomePage);
    lv_obj_set_size(ui_BatArc, 50, 50);
    lv_obj_set_x(ui_BatArc, -75);
    lv_obj_set_y(ui_BatArc, -68);
    lv_obj_set_align(ui_BatArc, LV_ALIGN_CENTER);
    /* 圆弧默认可点击拖动，电量显示不需要交互 */
    lv_obj_clear_flag(ui_BatArc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_value(ui_BatArc, ui_BatArcValue);
    lv_obj_set_style_arc_width(ui_BatArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_BatArc, lv_color_hex(0xE11432), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_BatArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    /* 圆点（KNOB）也涂成绿色，并去掉内边距 */
    lv_obj_set_style_bg_color(ui_BatArc, lv_color_hex(0xE11432), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BatArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_BatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_BaticonLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_BaticonLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BaticonLabel, -75);
    lv_obj_set_y(ui_BaticonLabel, -50);
    lv_obj_set_align(ui_BaticonLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_BaticonLabel, ICON_BAT);
    lv_obj_set_style_text_color(ui_BaticonLabel, lv_color_hex(0xE11432), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BaticonLabel, &ui_font_iconfont16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BatNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_BatNumLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BatNumLabel, -75);
    lv_obj_set_y(ui_BatNumLabel, -68);
    lv_obj_set_align(ui_BatNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%2d%%", ui_BatArcValue);
    lv_label_set_text(ui_BatNumLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_BatNumLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 日期：11-08 Tue. ---------- */

    ui_DateLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_DateLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_DateLabel, -75);
    lv_obj_set_y(ui_DateLabel, -110);
    lv_obj_set_align(ui_DateLabel, LV_ALIGN_RIGHT_MID);
    sprintf(value_strbuf, "%2d-%02d", ui_DateMonthValue, ui_DateDayValue);
    lv_label_set_text(ui_DateLabel, value_strbuf);
    lv_obj_set_style_text_color(ui_DateLabel, lv_color_hex(0xFF461E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_DateLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_DayLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_DayLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_DayLabel, -20);
    lv_obj_set_y(ui_DayLabel, -110);
    lv_obj_set_align(ui_DayLabel, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_DayLabel, ui_Days[ui_DataWeekdayValue - 1]);
    lv_obj_set_style_text_color(ui_DayLabel, lv_color_hex(0xFF461E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_DayLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---------- 步数：图标 + 文案 + 数字 + 进度条 ---------- */

    ui_StepiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_StepiconLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StepiconLabel, -90);
    lv_obj_set_y(ui_StepiconLabel, -5);
    lv_obj_set_align(ui_StepiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StepiconLabel, ICON_STEP);
    lv_obj_set_style_text_color(ui_StepiconLabel, lv_color_hex(0x3278FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StepiconLabel, &ui_font_iconfont24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StepCnLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_StepCnLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StepCnLabel, -40);
    lv_obj_set_y(ui_StepCnLabel, -5);
    lv_obj_set_align(ui_StepCnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StepCnLabel, TEXT_STEP_CN);
    lv_obj_set_style_text_color(ui_StepCnLabel, lv_color_hex(0x3278FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StepCnLabel, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StepNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_StepNumLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StepNumLabel, -80);
    lv_obj_set_y(ui_StepNumLabel, 20);
    lv_obj_set_align(ui_StepNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%d", ui_StepNumValue);
    lv_label_set_text(ui_StepNumLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_StepNumLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StepNumBar = lv_bar_create(ui_HomePage);
    lv_bar_set_value(ui_StepNumBar, 40, LV_ANIM_OFF);
    lv_obj_set_size(ui_StepNumBar, 200, 10);
    lv_obj_set_x(ui_StepNumBar, 0);
    lv_obj_set_y(ui_StepNumBar, 50);
    lv_obj_set_align(ui_StepNumBar, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_StepNumBar, lv_color_hex(0x3278FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);

    /* ---------- 温度：25，范围 -20~60 ---------- */

    ui_TempArc = lv_arc_create(ui_HomePage);
    lv_obj_set_size(ui_TempArc, 50, 50);
    lv_obj_set_x(ui_TempArc, -75);
    lv_obj_set_y(ui_TempArc, 90);
    lv_obj_set_align(ui_TempArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_TempArc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_range(ui_TempArc, -20, 60);
    lv_arc_set_value(ui_TempArc, ui_HomePageTempValue);
    lv_obj_set_style_arc_width(ui_TempArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_TempArc, lv_color_hex(0xF5A73A), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_TempArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_TempArc, lv_color_hex(0xF5A73A), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TempArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_TempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_TempiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_TempiconLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_TempiconLabel, -75);
    lv_obj_set_y(ui_TempiconLabel, 90);
    lv_obj_set_align(ui_TempiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_TempiconLabel, ICON_TEMP);
    lv_obj_set_style_text_color(ui_TempiconLabel, lv_color_hex(0xF5A73A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TempiconLabel, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TempNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_TempNumLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_TempNumLabel, -73);
    lv_obj_set_y(ui_TempNumLabel, 125);
    lv_obj_set_align(ui_TempNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%d", ui_HomePageTempValue);
    lv_label_set_text(ui_TempNumLabel, value_strbuf);

    /* ---------- 湿度：67，圆弧留 10° 缺口 ---------- */

    ui_HumiiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_HumiiconLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_HumiiconLabel, 0);
    lv_obj_set_y(ui_HumiiconLabel, 89);
    lv_obj_set_align(ui_HumiiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_HumiiconLabel, ICON_HUMI);
    lv_obj_set_style_text_color(ui_HumiiconLabel, lv_color_hex(0x14C8E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HumiiconLabel, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HumiArc = lv_arc_create(ui_HomePage);
    lv_obj_set_size(ui_HumiArc, 50, 50);
    lv_obj_set_x(ui_HumiArc, 0);
    lv_obj_set_y(ui_HumiArc, 90);
    lv_obj_set_align(ui_HumiArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_HumiArc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_value(ui_HumiArc, ui_HomePageHumiValue);
    lv_arc_set_bg_angles(ui_HumiArc, 270, 620);
    lv_obj_set_style_arc_width(ui_HumiArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_HumiArc, lv_color_hex(0x14C8E1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_HumiArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_HumiArc, lv_color_hex(0x14C8E1), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HumiArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_HumiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_HumiNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_HumiNumLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_HumiNumLabel, 0);
    lv_obj_set_y(ui_HumiNumLabel, 125);
    lv_obj_set_align(ui_HumiNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%d", ui_HomePageHumiValue);
    lv_label_set_text(ui_HumiNumLabel, value_strbuf);

    /* ---------- 心率：72 ---------- */

    ui_HRArc = lv_arc_create(ui_HomePage);
    lv_obj_set_size(ui_HRArc, 50, 50);
    lv_obj_set_x(ui_HRArc, 70);
    lv_obj_set_y(ui_HRArc, 90);
    lv_obj_set_align(ui_HRArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_HRArc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_value(ui_HRArc, 100);
    lv_arc_set_bg_angles(ui_HRArc, 270, 620);
    lv_obj_set_style_arc_width(ui_HRArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_HRArc, lv_color_hex(0xE11432), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_HRArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_HRArc, lv_color_hex(0xE11432), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HRArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_HRArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_HRiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_HRiconLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_HRiconLabel, 70);
    lv_obj_set_y(ui_HRiconLabel, 90);
    lv_obj_set_align(ui_HRiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_HRiconLabel, ICON_HR);
    lv_obj_set_style_text_color(ui_HRiconLabel, lv_color_hex(0xE11432), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HRiconLabel, &ui_font_iconfont32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HRNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_size(ui_HRNumLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_HRNumLabel, 70);
    lv_obj_set_y(ui_HRNumLabel, 125);
    lv_obj_set_align(ui_HRNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%d", ui_HomePageHRValue);
    lv_label_set_text(ui_HRNumLabel, value_strbuf);

    /* ---------- 练习 5：左上角一个普通 label ---------- */
    lv_obj_t *hello = lv_label_create(ui_HomePage);
    lv_label_set_text(hello, "HELLO");
    lv_obj_set_style_text_color(hello, lv_color_white(), 0);
    lv_obj_set_style_text_font(hello, &lv_font_montserrat_20, 0);
    lv_obj_align(hello, LV_ALIGN_TOP_LEFT, 8, 8);

    /* ---------- 下拉面板（1.4 课）：放最后创建，才会盖在其它控件上面 ---------- */
    HomePage_create_dropdown_panel();

    /* ---------- 每 500 ms 刷新一次（和参考工程一致） ---------- */
    ui_HomePageTimer = lv_timer_create(HomePage_timer_cb, 500, NULL);
}

/*
 * 页面销毁时必须删掉自己创建的定时器。
 * 否则定时器还会继续运行，回调里去访问已经被释放的 label，程序就会崩。
 * 屏幕对象本身由页面栈的 lv_scr_load_anim(..., true) 自动释放。
 *
 * 下拉面板不用在这里删：它是 ui_HomePage 的子对象，删屏幕时 LVGL 会连它
 * 和里面所有按钮一起销毁；挂在它们上面的回调也随之消失。
 * （1.4 课新增的六个 lv_obj_add_event_cb 都属于这种情况。）
 */
void ui_HomePage_screen_deinit(void)
{
    if (ui_HomePageTimer != NULL) {
        lv_timer_del(ui_HomePageTimer);
        ui_HomePageTimer = NULL;
    }
}
