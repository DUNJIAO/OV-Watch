#include "ui.h"
#include "Screens/Inc/ui_HomePage.h"
#include "Screens/Inc/ui_MenuPage.h"

/*
 * 这里是“接线”代码，不是练习内容：
 *   设置主题 -> 把首页交给页面栈 -> 起一个 1 秒的主定时器
 *
 * 2.1 课之前这里是 ui_HomePage_screen_init() + lv_disp_load_scr()，
 * 现在换成 Pages_init()：建哪一页、显示哪一页，都由 PageManager 决定。
 */

/* 主定时器：参考工程里它现在是空的，留着是给阶段 6 的任务层挂刷新用的 */
static void main_timer(lv_timer_t *timer)
{
    (void)timer;
}

void ui_init(void)
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp,
                                              lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED),
                                              true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    Pages_init();

    lv_timer_create(main_timer, 1000, NULL);
}
