/**
 * OV-Watch 学习工程 —— PC 仿真入口
 *
 * 这部分是让你能“写完就看见”的架子，不用自己写。
 * 它做的事和参考工程 lv_sim_vscode_win/main.c 一样：
 *   1. 初始化 LVGL
 *   2. 注册 SDL 显示 / 鼠标 / 键盘 / 滚轮输入
 *   3. 调用你自己的 ui_init()
 *   4. 循环调用 lv_timer_handler()
 */
#define _DEFAULT_SOURCE /* usleep() */
#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>   /* SetConsoleOutputCP：让控制台按 UTF-8 显示中文 */
#endif
#include <SDL2/SDL.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/sdl/sdl.h"
#include "ui.h"
#include "Func/Inc/HWDataAccess.h"

/* 手表屏幕分辨率，和硬件保持一致 */
#define HOR_RES 240
#define VER_RES 280

static void hal_init(void);

int main(void)
{
    /* PC 专用：关掉标准输出的缓冲，printf 立刻出现在控制台。
     * 不关的话重定向到文件时，要等缓冲区满了才看得见。 */
    setvbuf(stdout, NULL, _IONBF, 0);

#ifdef _WIN32
    /* 源码是 UTF-8，中文控制台默认却是 GBK，不改这一句 printf 出来的
     * 中文会是乱码。重定向到文件时这句没有副作用。 */
    SetConsoleOutputCP(CP_UTF8);
#endif

    lv_init();
    hal_init();

    /* 实机上这些由 InitTask 调用；PC 上是空函数或直接返回成功 */
    HWInterface.IMU.Init();
    HWInterface.AHT21.Init();
    HWInterface.HR_meter.Init();
    HWInterface.Power.Init();

    /* 仿真数据先刷一次，界面第一帧拿到的就是"真实"值 */
    HW_Sim_Update();

    ui_init(); /* 你的界面从这里开始 */

    while (1) {
        HW_Sim_Update();   /* 仿真专用：代替实机上的传感器任务，每秒更新一次数据 */
        lv_timer_handler();
        usleep(5 * 1000);
    }
    return 0;
}

static void hal_init(void)
{
    /* 用 SDL 在 PC 上模拟一块屏幕 */
    sdl_init();

    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf[HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, HOR_RES * 100);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf  = &disp_buf;
    disp_drv.flush_cb  = sdl_display_flush;
    disp_drv.hor_res   = HOR_RES;
    disp_drv.ver_res   = VER_RES;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    /* 默认主题 */
    lv_theme_t *th = lv_theme_default_init(disp,
                                           lv_palette_main(LV_PALETTE_BLUE),
                                           lv_palette_main(LV_PALETTE_RED),
                                           LV_THEME_DEFAULT_DARK,
                                           LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    /* 鼠标（触摸屏） */
    static lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type    = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = sdl_mouse_read;
    lv_indev_t *mouse = lv_indev_drv_register(&mouse_drv);

    /* 键盘 */
    static lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type    = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = sdl_keyboard_read;
    lv_indev_t *kb = lv_indev_drv_register(&kb_drv);
    lv_indev_set_group(kb, g);

    /* 编码器 / 滚轮 */
    static lv_indev_drv_t enc_drv;
    lv_indev_drv_init(&enc_drv);
    enc_drv.type    = LV_INDEV_TYPE_ENCODER;
    enc_drv.read_cb = sdl_mousewheel_read;
    lv_indev_t *enc = lv_indev_drv_register(&enc_drv);
    lv_indev_set_group(enc, g);

    /* 鼠标指针图案 */
    LV_IMG_DECLARE(mouse_cursor_icon);
    lv_obj_t *cursor = lv_img_create(lv_scr_act());
    lv_img_set_src(cursor, &mouse_cursor_icon);
    lv_indev_set_cursor(mouse, cursor);
}
