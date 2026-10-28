/**
 * 硬件抽象层实现
 *
 * 对应参考实现：
 *   Software/OV_Watch/User/Func/Src/HWDataAccess.c
 *
 * 文件结构照抄参考工程：
 *   上半部分   每个外设一组 HW_xxx_函数
 *              每个函数里面用 #if HW_USE_XXX 决定调 BSP 还是返回仿真数据
 *   最后       用函数指针把 HWInterface 填起来
 *
 * 你只需要看懂「为什么这么分层」，不需要背每个函数。
 */
#include "../Inc/HWDataAccess.h"

#if !HW_USE_HARDWARE
  #include <time.h>
  #include <stdio.h>   /* 仿真分支用 printf 在控制台"说话"，方便确认抽象层被调到了 */
#endif

/* ==========================================================================
 *  PC 仿真数据源
 * ========================================================================== */
#if !HW_USE_HARDWARE

/*
 * 时间倍速：
 *   1  = 和电脑时间完全一致（窗口气和任务栏对得上，用来验证抽象层接通了）
 *   60 = 快 60 倍，方便看分钟跳动
 */
#define SIM_TIME_SPEED     1

#define SIM_BAT_START      87   /* 起始电量 % */
#define SIM_BAT_DRAIN_SEC  30   /* 每多少秒掉 1% */
#define SIM_STEP_PER_SEC   3    /* 每秒走几步 */

static time_t   sim_start_real = 0;          /* 程序启动那一刻的真实时间 */
static time_t   sim_base       = 0;          /* 手表 RTC 的"零点" */
static time_t   sim_now        = 0;          /* 手表 RTC 当前读数 */
static unsigned sim_elapsed    = 0xFFFFFFFFu;/* 启动后经过的秒数（初值保证第一次一定刷新） */

static void sim_init(void)
{
    if (sim_start_real != 0) {
        return;
    }
    sim_start_real = time(NULL);
    sim_base       = sim_start_real;
    sim_now        = sim_start_real;
}

/* 直接改 RTC 读数（SetDate / SetTime 用），同时把基准点挪过去保持匀速 */
static void sim_set_now(time_t t)
{
    sim_base = t - (time_t)(sim_elapsed * SIM_TIME_SPEED);
    sim_now  = t;
}

void HW_Sim_Update(void)
{
    time_t   real_now;
    unsigned elapsed;
    int      tri;       /* 三角波 -20 ~ +19，让传感器数值慢慢来回变 */
    int      bat;

    sim_init();

    real_now = time(NULL);
    elapsed  = (unsigned)(real_now - sim_start_real);
    if (elapsed == sim_elapsed) {
        return;         /* 这一秒已经刷过了 */
    }
    sim_elapsed = elapsed;

    /* RTC：从启动时刻开始匀速前进 */
    sim_now = sim_base + (time_t)(elapsed * SIM_TIME_SPEED);

    /* 电量：慢慢掉 */
    bat = (int)SIM_BAT_START - (int)(elapsed / SIM_BAT_DRAIN_SEC);
    if (bat < 5) {
        bat = 5;        /* 别掉到 0，看着难受 */
    }
    HWInterface.Power.power_remain = (uint8_t)bat;

    /* 步数：一直涨 */
    {
        unsigned steps = elapsed * SIM_STEP_PER_SEC;
        if (steps > 65000u) {
            steps = 65000u;
        }
        HWInterface.IMU.Steps = (uint16_t)steps;
    }

    /* 温湿度：25℃ / 67% 附近小幅摆动 */
    tri = (int)((elapsed / 5u) % 40u) - 20;   /* -20 ... +19 */
    HWInterface.AHT21.temperature = (uint8_t)(25 + tri / 10);   /* 23 ... 26 */
    HWInterface.AHT21.humidity    = (uint8_t)(67 + tri / 8);    /* 65 ... 69 */

    /* 心率：66 ... 78 */
    HWInterface.HR_meter.HrRate = (uint8_t)(72 + tri / 3);
    HWInterface.HR_meter.SPO2   = (uint8_t)(98 + tri / 20);

    /* 气压计 / 罗盘 */
    HWInterface.Barometer.altitude  = (uint16_t)(19 + tri / 10);
    HWInterface.Ecompass.direction  = (uint16_t)((elapsed * 10u) % 360u);
}

#endif /* !HW_USE_HARDWARE */


/* ==========================================================================
 *  RTC
 * ========================================================================== */
void HW_RTC_Get_TimeDate(HW_DateTimeTypeDef *nowdatetime)
{
    if (nowdatetime == NULL) {
        return;
    }

#if HW_USE_RTC
    {
        RTC_DateTypeDef nowdate;
        RTC_TimeTypeDef nowtime;
        HAL_RTC_GetTime(&hrtc, &nowtime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &nowdate, RTC_FORMAT_BIN);
        nowdatetime->Year    = nowdate.Year;
        nowdatetime->Month   = nowdate.Month;
        nowdatetime->Date    = nowdate.Date;
        nowdatetime->WeekDay = weekday_calculate(nowdatetime->Year,
                                                 nowdatetime->Month,
                                                 nowdatetime->Date, 20);
        nowdatetime->Hours   = nowtime.Hours;
        nowdatetime->Minutes = nowtime.Minutes;
        nowdatetime->Seconds = nowtime.Seconds;
    }
#else
    {
        struct tm *t;
        sim_init();
        t = localtime(&sim_now);
        if (t == NULL) {
            return;
        }
        nowdatetime->Year    = (uint8_t)(t->tm_year % 100);
        nowdatetime->Month   = (uint8_t)(t->tm_mon + 1);
        nowdatetime->Date    = (uint8_t)t->tm_mday;
        /* localtime: 0 = 周日；我们要 1 = 周一 ... 7 = 周日 */
        nowdatetime->WeekDay = (uint8_t)(t->tm_wday == 0 ? 7 : t->tm_wday);
        nowdatetime->Hours   = (uint8_t)t->tm_hour;
        nowdatetime->Minutes = (uint8_t)t->tm_min;
        nowdatetime->Seconds = (uint8_t)t->tm_sec;
    }
#endif
}

void HW_RTC_Set_Date(uint8_t year, uint8_t month, uint8_t date)
{
#if HW_USE_RTC
    RTC_SetDate(year, month, date);
#else
    struct tm t;
    sim_init();
    t = *localtime(&sim_now);
    t.tm_year = 100 + year;     /* tm_year 是「1900 年后的年数」 */
    t.tm_mon  = month - 1;      /* tm_mon 从 0 开始 */
    t.tm_mday = date;
    sim_set_now(mktime(&t));
#endif
}

void HW_RTC_Set_Time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
#if HW_USE_RTC
    RTC_SetTime(hours, minutes, seconds);
#else
    struct tm t;
    sim_init();
    t = *localtime(&sim_now);
    t.tm_hour = hours;
    t.tm_min  = minutes;
    t.tm_sec  = seconds;
    sim_set_now(mktime(&t));
#endif
}

/* 蔡勒公式算星期几：返回 1 = 周一 ... 7 = 周日 */
uint8_t HW_weekday_calculate(uint8_t setyear, uint8_t setmonth, uint8_t setday, uint8_t century)
{
    int w;

    if (setmonth == 1 || setmonth == 2) {
        setyear--;
        setmonth += 12;     /* 1、2 月当作上一年的 13、14 月 */
    }
    w = setyear + setyear / 4 + century / 4
        + 26 * (setmonth + 1) / 10 + setday - 1 - 2 * century;
    while (w < 0) {
        w += 7;
    }
    w %= 7;
    return (uint8_t)((w == 0) ? 7 : w);
}


/* ==========================================================================
 *  Power
 * ========================================================================== */
void HW_Power_Init(void)
{
#if HW_USE_BAT
    Power_Init();
#endif
}

void HW_Power_Shutdown(void)
{
#if HW_USE_BAT
    Power_DisEnable();
#else
    /* 仿真：不能真的断电，就在控制台说一声（1.4 课：电源按钮会走到这里） */
    printf("[sim] Power.Shutdown()  —— 真机上这里会拉低电源芯片的使能脚\n");
#endif
}

uint8_t HW_Power_BatCalculate(void)
{
#if HW_USE_BAT
    return PowerCalculate();
#else
    /* 仿真：电量的变化已经在 HW_Sim_Update() 里算好了，这里只是读回来。
     * 参考工程在没硬件时直接 return 0。 */
    return HWInterface.Power.power_remain;
#endif
}


/* ==========================================================================
 *  BLE
 * ========================================================================== */
void HW_BLE_Init(void)
{
#if HW_USE_BLE
    KT6328_GPIO_Init();
#endif
}

void HW_BLE_Enable(void)
{
#if HW_USE_BLE
    KT6328_Enable();
#else
    printf("[sim] BLE.Enable()   —— 真机上给 KT6328 上电使能\n");
#endif
}

void HW_BLE_Disable(void)
{
#if HW_USE_BLE
    KT6328_Disable();
#else
    printf("[sim] BLE.Disable()  —— 真机上关掉 KT6328\n");
#endif
}


/* ==========================================================================
 *  LCD 背光
 * ========================================================================== */
void HW_LCD_Set_Light(uint8_t dc)
{
#if HW_USE_LCD
    LCD_Set_Light(dc);
#else
    /* 仿真：SDL 窗口没有背光可调，只能打印出来证明请求到了这里 */
    printf("[sim] LCD.SetLight(%u) —— 真机上改的是背光 PWM 占空比\n", (unsigned)dc);
#endif
}


/* ==========================================================================
 *  IMU（MPU6050：计步、抬腕）
 * ========================================================================== */
uint8_t HW_MPU_Init(void)
{
#if HW_USE_IMU
    return mpu_dmp_init();
#else
    return 0;   /* 仿真：当作初始化成功 */
#endif
}

void HW_MPU_Wrist_Enable(void)
{
    HWInterface.IMU.wrist_is_enabled = 1;
}

void HW_MPU_Wrist_Disable(void)
{
    HWInterface.IMU.wrist_is_enabled = 0;
}

uint16_t HW_MPU_Get_Steps(void)
{
#if HW_USE_IMU
    unsigned long steps = 0;
    if (!HWInterface.IMU.ConnectionError) {
        dmp_get_pedometer_step_count(&steps);
    }
    return (uint16_t)steps;
#else
    return HWInterface.IMU.Steps;
#endif
}

int HW_MPU_Set_Steps(unsigned long count)
{
#if HW_USE_IMU
    if (!HWInterface.IMU.ConnectionError) {
        return dmp_set_pedometer_step_count(count);
    }
    return -1;
#else
    HWInterface.IMU.Steps = (uint16_t)count;
    return 0;
#endif
}


/* ==========================================================================
 *  AHT21（温湿度）
 * ========================================================================== */
uint8_t HW_AHT21_Init(void)
{
#if HW_USE_AHT21
    return AHT_Init();
#else
    return 0;
#endif
}

void HW_AHT21_Get_Humi_Temp(float *humi, float *temp)
{
#if HW_USE_AHT21
    if (!HWInterface.AHT21.ConnectionError) {
        AHT_Read(humi, temp);
    }
#else
    if (humi != NULL) {
        *humi = (float)HWInterface.AHT21.humidity;
    }
    if (temp != NULL) {
        *temp = (float)HWInterface.AHT21.temperature;
    }
#endif
}


/* ==========================================================================
 *  SPL06-001（气压计）
 * ========================================================================== */
uint8_t HW_Barometer_Init(void)
{
#if HW_USE_SPL06
    return SPL_init();
#else
    return 0;
#endif
}


/* ==========================================================================
 *  LSM303（电子罗盘）
 * ========================================================================== */
uint8_t HW_Ecompass_Init(void)
{
#if HW_USE_LSM303
    return LSM303DLH_Init();
#else
    return 0;
#endif
}

void HW_Ecompass_Sleep(void)
{
#if HW_USE_LSM303
    LSM303DLH_Sleep();
#endif
}


/* ==========================================================================
 *  EM7028（心率 / 血氧）
 * ========================================================================== */
uint8_t HW_HRmeter_Init(void)
{
#if HW_USE_EM7028
    return EM7028_hrs_init();
#else
    return 0;
#endif
}

void HW_HRmeter_Sleep(void)
{
#if HW_USE_EM7028
    EM7028_hrs_DisEnable();
#endif
}


/* ==========================================================================
 *  全局变量：把上面这些函数挂到 HWInterface 上
 *
 *  界面写 HWInterface.Power.power_remain 时，编译器把它翻译成
 *  「取 power_remain 这个字段」——不关心背后是真实电池还是仿真。
 *  界面写 HWInterface.RealTimeClock.GetTimeDate(&dt) 时，
 *  实际执行的是 HW_RTC_Get_TimeDate()，里面 #if 决定读硬件还是读 localtime。
 * ========================================================================== */
HW_InterfaceTypeDef HWInterface = {
    .RealTimeClock = {
        .GetTimeDate      = HW_RTC_Get_TimeDate,
        .SetDate          = HW_RTC_Set_Date,
        .SetTime          = HW_RTC_Set_Time,
        .CalculateWeekday = HW_weekday_calculate
    },
    .BLE = {
        .Init    = HW_BLE_Init,
        .Enable  = HW_BLE_Enable,
        .Disable = HW_BLE_Disable
    },
    .Power = {
        .power_remain = 0,
        .Init         = HW_Power_Init,
        .Shutdown     = HW_Power_Shutdown,
        .BatCalculate = HW_Power_BatCalculate
    },
    .LCD = {
        .SetLight = HW_LCD_Set_Light
    },
    .IMU = {
        /* 参考工程初始化时写 1（还没连上），由任务检测成功后改成 0。
         * 仿真里一开始就是"连上了"，所以写 0。 */
        .ConnectionError   = 0,
        .Steps             = 0,
        .wrist_is_enabled  = 0,
        .wrist_state       = WRIST_UP,
        .Init              = HW_MPU_Init,
        .WristEnable       = HW_MPU_Wrist_Enable,
        .WristDisable      = HW_MPU_Wrist_Disable,
        .GetSteps          = HW_MPU_Get_Steps,
        .SetSteps          = HW_MPU_Set_Steps
    },
    .AHT21 = {
        .ConnectionError = 0,
        .humidity        = 67,
        .temperature     = 26,
        .Init            = HW_AHT21_Init,
        .GetHumiTemp     = HW_AHT21_Get_Humi_Temp
    },
    .Barometer = {
        .ConnectionError = 0,
        .altitude        = 19,
        .Init            = HW_Barometer_Init
    },
    .Ecompass = {
        .ConnectionError = 0,
        .direction       = 45,
        .Init            = HW_Ecompass_Init,
        .Sleep           = HW_Ecompass_Sleep
    },
    .HR_meter = {
        .ConnectionError = 0,
        .HrRate          = 0,
        .SPO2            = 99,
        .Init            = HW_HRmeter_Init,
        .Sleep           = HW_HRmeter_Sleep
    }
};
