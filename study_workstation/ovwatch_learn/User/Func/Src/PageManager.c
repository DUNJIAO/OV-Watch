/*
 * 页面栈（PageManager）
 *
 * 对应参考实现：Software/OV_Watch/User/Func/Src/PageManager.c
 *
 * 屏幕上同一时刻只能显示一页，但用户走过的路要记住：
 *
 *        栈底                    栈顶
 *      [ Home ] --> [ Menu ] --> [ Set ]
 *                                    ^ 现在显示这一页
 *
 * 进入新页面 = 入栈（push）；按下返回 = 出栈（pop），把下面那一页重新初始化。
 * 整个"当前在哪、怎么回去"的逻辑，就这 100 行。
 *
 * 页面的初始化函数由各页面自己提供（ui_xxx_screen_init），
 * 这里只调用、不关心里面画了什么——所以加一个新页面不用改这个文件。
 */
#include "../Inc/PageManager.h"
#include "../../GUI_App/Screens/Inc/ui_HomePage.h"
#include "../../GUI_App/Screens/Inc/ui_MenuPage.h"

PageStack_t PageStack;

/* ---------- 栈的四个基本操作（static：外面只能通过下面的接口用） ---------- */

static void page_stack_init(PageStack_t *stack)
{
    stack->top = 0;
}

/* 入栈。返回值 0 表示成功；函数是 uint8_t，所以 -1 实际是 255（见第 5 课） */
static uint8_t page_stack_push(PageStack_t *stack, Page_t *page)
{
    if (stack->top >= MAX_DEPTH) {
        return -1;
    }
    stack->pages[stack->top++] = page;
    return 0;
}

/* 出栈。注意：弹出之前会先让这一页自己 deinit（删定时器等） */
static uint8_t page_stack_pop(PageStack_t *stack)
{
    if (stack->top <= 0) {
        return -1;
    }
    stack->pages[--stack->top]->deinit();
    return 0;
}

static uint8_t page_stack_is_empty(const PageStack_t *stack)
{
    return stack->top == 0;
}

/* 栈顶页面；栈空时返回 NULL */
static Page_t *get_top_page(PageStack_t *stack)
{
    if (stack->top == 0) {
        return NULL;
    }
    return stack->pages[stack->top - 1];
}

/* ---------- 对外接口 ---------- */

/* 现在显示的是哪一页。各页面的事件回调用它判断"这条事件是不是发给我的" */
Page_t *Page_Get_NowPage(void)
{
    return get_top_page(&PageStack);
}

/*
 * 返回上一页。
 *
 * 先弹出当前页，再看栈里还剩什么：
 *   还剩页面 -> 重新初始化它（因为离开时屏幕已被自动删除），滑回来
 *   已经空了 -> 理论上走不到（首页永远不会被弹出），参考实现会重新压入首页和菜单页
 */
void Page_Back(void)
{
    if (page_stack_is_empty(&PageStack)) {
        return;
    }

    page_stack_pop(&PageStack);

    if (page_stack_is_empty(&PageStack)) {
        page_stack_push(&PageStack, &Page_Home);
        page_stack_push(&PageStack, &Page_Menu);
        Page_Menu.init();
        lv_scr_load_anim(*Page_Menu.page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    } else {
        Page_t *previous_page = PageStack.pages[PageStack.top - 1];
        previous_page->init();
        lv_scr_load_anim(*previous_page->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    }
}

/* 一路退回首页：把栈顶到栈底之间的页面全弹掉 */
void Page_Back_Bottom(void)
{
    if (page_stack_is_empty(&PageStack)) {
        return;
    }

    while (PageStack.top > 1) {
        page_stack_pop(&PageStack);
    }
    PageStack.pages[PageStack.top - 1]->init();
    lv_scr_load_anim(*PageStack.pages[PageStack.top - 1]->page_obj,
                     LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
}

/*
 * 打开一个新页面。
 *
 * 顺序很重要：
 *   1. 先让当前页 deinit —— 它的定时器必须在屏幕消失前停掉
 *   2. 新页入栈
 *   3. 新页 init —— 建控件、建定时器
 *   4. 切换屏幕，auto_del = true（第三个 true 不是"开动画"，见第 5 课）
 */
void Page_Load(Page_t *newPage)
{
    if (PageStack.top >= MAX_DEPTH - 1) {
        return;
    }

    if (PageStack.top > 0) {
        PageStack.pages[PageStack.top - 1]->deinit();
    }

    page_stack_push(&PageStack, newPage);
    newPage->init();
    lv_scr_load_anim(*newPage->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
}

/* 开机时调用：清空栈，把首页压到栈底并显示 */
void Pages_init(void)
{
    page_stack_init(&PageStack);
    page_stack_push(&PageStack, &Page_Home);
    Page_Home.init();
    lv_disp_load_scr(*Page_Home.page_obj);
}
