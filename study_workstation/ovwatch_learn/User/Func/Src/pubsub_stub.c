/*
 * ===== 基建占位文件，不是课程内容 =====
 *
 * 参考工程的 lv_drivers/sdl/sdl.c 里写死了对下面两个符号的引用：
 *     SDL_KeyBoard_Publisher
 *     Publisher_publish()
 * 它是给 PC 键盘的退格键发消息用的。我们的工程不编译参考工程的 user_test，
 * 所以先在这里补两个空实现，让链接通过。
 *
 * 单元 2.2 做「发布/订阅」时删掉这个文件，
 * 换成 User/Func/Src/pubsub.c 的完整实现。
 */
#include "Func/Inc/pubsub.h"

PubSub_Publisher_t SDL_KeyBoard_Publisher;

void Publisher_publish(PubSub_Publisher_t *publisher, PubSub_Message_t message)
{
    (void)publisher;
    (void)message;
}
