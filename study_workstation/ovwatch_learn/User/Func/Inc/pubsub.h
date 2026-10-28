#ifndef PUBSUB_H
#define PUBSUB_H

#include <stdint.h>

/* 订阅/发布用的消息 */
typedef struct {
    int id;
    char data[256];
} PubSub_Message_t;

typedef void (*Subscriber)(PubSub_Message_t);

typedef struct SubscriberNode {
    Subscriber subscriber;
    struct SubscriberNode *next;
} SubscriberNode;

typedef struct {
    SubscriberNode *subscribers;
} PubSub_Publisher_t;

void Publisher_init(PubSub_Publisher_t *publisher);
void Publisher_subscribe(PubSub_Publisher_t *publisher, Subscriber subscriber);
void Publisher_unsubscribe(PubSub_Publisher_t *publisher, Subscriber subscriber);
void Publisher_publish(PubSub_Publisher_t *publisher, PubSub_Message_t message);

/* 仿真器把 PC 键盘事件（退格键）通过这个发布者送进来 */
extern PubSub_Publisher_t SDL_KeyBoard_Publisher;

#endif
