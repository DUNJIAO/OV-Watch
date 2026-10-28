#ifndef APP_UI_H
#define APP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include <stdio.h>

/*
 * 2.1 课：所有页面都要用 Page_t，所以从 ui.h 里统一带出来。
 * （PageManager.h 自己也会 include 本文件，靠头文件保护符不会循环展开。）
 */
#include "../Func/Inc/PageManager.h"

/*
 * 自定义字体（由 SquareLine Studio 生成）
 * 源文件在参考工程 user_test/GUI_App/Fonts/ 下，Makefile 会一起编译。
 */
LV_FONT_DECLARE(ui_font_Cuyuan18);
LV_FONT_DECLARE(ui_font_Cuyuan20);
LV_FONT_DECLARE(ui_font_Cuyuan24);
LV_FONT_DECLARE(ui_font_Cuyuan30);
LV_FONT_DECLARE(ui_font_Cuyuan38);
LV_FONT_DECLARE(ui_font_Cuyuan48);
LV_FONT_DECLARE(ui_font_Cuyuan80);
LV_FONT_DECLARE(ui_font_Cuyuan100);
LV_FONT_DECLARE(ui_font_iconfont16);
LV_FONT_DECLARE(ui_font_iconfont24);
LV_FONT_DECLARE(ui_font_iconfont28);
LV_FONT_DECLARE(ui_font_iconfont30);
LV_FONT_DECLARE(ui_font_iconfont32);
LV_FONT_DECLARE(ui_font_iconfont34);
LV_FONT_DECLARE(ui_font_iconfont45);

/* 整个界面的入口：创建页面并显示第一屏 */
void ui_init(void);

#ifdef __cplusplus
}
#endif

#endif
