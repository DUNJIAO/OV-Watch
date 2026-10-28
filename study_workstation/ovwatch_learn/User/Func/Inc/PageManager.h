#ifndef PAGE_STACK_H
#define PAGE_STACK_H

#include "../../GUI_App/ui.h"

/*
 * 页面栈最多放几页。
 *
 * 注意 Page_Load 里判的是 top >= MAX_DEPTH - 1，所以实际最多 5 页，
 * 第 6 个格子是参考工程留的余量（它这么写，我们照抄，行为一致）。
 */
#define MAX_DEPTH 6

/*
 * 一个页面 = 三个东西：
 *
 *   init      创建这一页的所有控件，并把屏幕指针写进 page_obj
 *   deinit    离开这一页时，回收它自己申请的资源（主要是定时器）
 *   page_obj  指向页面自己创建的 lv_obj_t 屏幕（二级指针，因为创建时会改写）
 *
 * 注意 init 会被调用很多次：每次回到这一页都会重建整页（原因见第 5 课）。
 * 所以 init 里不能依赖"只执行一次"的假设。
 */
typedef struct {
    void (*init)(void);
    void (*deinit)(void);
    lv_obj_t **page_obj;
} Page_t;

/* 栈：pages[0] 是最底下那一页，pages[top - 1] 是正在显示的页面 */
typedef struct {
    Page_t *pages[MAX_DEPTH];
    uint8_t top;            /* 页数，同时也是"下一个空位"的下标 */
} PageStack_t;

extern PageStack_t PageStack;

Page_t *Page_Get_NowPage(void);
void Page_Back(void);
void Page_Back_Bottom(void);
void Page_Load(Page_t *newPage);
void Pages_init(void);

#endif
