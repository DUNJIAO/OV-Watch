/*
 * 菜单页（2.1 课：占位版本）
 *
 * 真正的菜单是 2.3 课的事。这一页现在只干三件事：
 *   1. 证明"入栈"真的切了屏幕
 *   2. 把栈深和"第几次打开"写在屏幕上，让 init/deinit 看得见
 *   3. 提供一个返回按钮，证明"出栈"能回到上一页
 *
 * 有一处是故意不删的：菜单打开次数 menu_open_count 是 static。
 * 每进一次这个页面，init 就会被调用一次，计数加一。
 * 看到数字涨，就说明"页面不是藏起来了，是重新建了一遍"。
 *
 * 为什么这一页是英文？
 *   GUI_App/Fonts 里的 CuyuanXX 都是 SquareLine 按页裁出来的子集，
 *   比如 Cuyuan18 只含「改写选择开发中今日步数」这几个字，Cuyuan30 只有数字。
 *   写"菜单""返回主页"这种新中文，字库里根本没有对应字形，屏幕上就是一片空白。
 *   想加中文得重新导字体（lv_font_conv / SquareLine），那是 2.3 课做真菜单时的事。
 *   这里先用 LVGL 自带点阵字 montserrat，只写 ASCII。
 */
#include "../Inc/ui_MenuPage.h"

lv_obj_t *ui_MenuPage = NULL;

Page_t Page_Menu = {ui_MenuPage_screen_init, ui_MenuPage_screen_deinit, &ui_MenuPage};

static uint32_t menu_open_count = 0;

static void ui_event_MenuPage(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    /* 出栈，回到上一页（这里是主页） */
    Page_Back();
}

void ui_MenuPage_screen_init(void)
{
    ui_MenuPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_MenuPage, LV_OBJ_FLAG_SCROLLABLE);

    menu_open_count++;

    lv_obj_t *title = lv_label_create(ui_MenuPage);
    lv_label_set_text(title, "MENU");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *note = lv_label_create(ui_MenuPage);
    lv_label_set_text(note, "placeholder for 2.3");
    lv_obj_set_style_text_font(note, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(note, LV_ALIGN_TOP_MID, 0, 78);

    /* 这两个数字是本课的"仪表盘" */
    lv_obj_t *count_label = lv_label_create(ui_MenuPage);
    lv_label_set_text_fmt(count_label, "open #%u", (unsigned)menu_open_count);
    lv_obj_set_style_text_font(count_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(count_label, LV_ALIGN_TOP_MID, 0, 108);

    lv_obj_t *depth_label = lv_label_create(ui_MenuPage);
    lv_label_set_text_fmt(depth_label, "stack top = %u", (unsigned)PageStack.top);
    lv_obj_set_style_text_font(depth_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(depth_label, LV_ALIGN_TOP_MID, 0, 138);

    /* 返回按钮：出栈 */
    lv_obj_t *back_btn = lv_btn_create(ui_MenuPage);
    lv_obj_set_size(back_btn, 140, 44);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -32);
    lv_obj_add_event_cb(back_btn, ui_event_MenuPage, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "BACK");
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(back_label);
}

/* 这一页没有定时器，也没有额外申请的东西，所以不用回收 */
void ui_MenuPage_screen_deinit(void)
{
}
