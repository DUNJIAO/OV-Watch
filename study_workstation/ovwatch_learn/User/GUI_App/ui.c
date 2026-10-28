#include "ui.h"
#include "Screens/Inc/ui_HomePage.h"

/*
 * 这里是“接线”代码，不是练习内容：
 *   设置主题 -> 创建主页 -> 把主页显示出来
 * 单元 2.1 做 PageManager 时会把它换成 Pages_init()。
 */
void ui_init(void)
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp,
                                              lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED),
                                              true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    ui_HomePage_screen_init();
    lv_disp_load_scr(ui_HomePage);
}
