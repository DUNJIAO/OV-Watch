#ifndef UI_MENUPAGE_H
#define UI_MENUPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

/*
 * 2.1 课的占位菜单页。
 *
 * 真正的菜单页（滚动列表 + 十几个入口）是 2.3 课的内容。
 * 这里只做一个能进能出的最小页面，专门用来验证页面栈：
 * 入栈时 init、出栈时 deinit、返回时再 init。
 *
 * 2.3 课会把这两个源文件整个替换掉，但 Page_t 的名字和签名不变。
 */
extern lv_obj_t *ui_MenuPage;

extern Page_t Page_Menu;

void ui_MenuPage_screen_init(void);
void ui_MenuPage_screen_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
