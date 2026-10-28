#ifndef UI_HOMEPAGE_H
#define UI_HOMEPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

/* 屏幕 */
extern lv_obj_t *ui_HomePage;

/* 时间 */
extern lv_obj_t *ui_TimeHourLabel;
extern lv_obj_t *ui_TimeColonLabel;
extern lv_obj_t *ui_TimeMinuteLabel;

/* 电量 */
extern lv_obj_t *ui_BatArc;
extern lv_obj_t *ui_BaticonLabel;
extern lv_obj_t *ui_BatNumLabel;

/* 日期 */
extern lv_obj_t *ui_DateLabel;
extern lv_obj_t *ui_DayLabel;

/* 步数 */
extern lv_obj_t *ui_StepiconLabel;
extern lv_obj_t *ui_StepCnLabel;
extern lv_obj_t *ui_StepNumLabel;
extern lv_obj_t *ui_StepNumBar;

/* 温度 */
extern lv_obj_t *ui_TempArc;
extern lv_obj_t *ui_TempiconLabel;
extern lv_obj_t *ui_TempNumLabel;

/* 湿度 */
extern lv_obj_t *ui_HumiiconLabel;
extern lv_obj_t *ui_HumiArc;
extern lv_obj_t *ui_HumiNumLabel;

/* 心率 */
extern lv_obj_t *ui_HRArc;
extern lv_obj_t *ui_HRiconLabel;
extern lv_obj_t *ui_HRNumLabel;

void ui_HomePage_screen_init(void);
void ui_HomePage_screen_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
