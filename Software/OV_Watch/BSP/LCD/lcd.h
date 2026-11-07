#ifndef __LCD_H
#define __LCD_H
#include "sys.h"

typedef void (*LCD_CallbackFunc_t)(void);

/**
  * @brief  在矩形区域内逐点填色
  * @param  xsta ysta 起始坐标（含）
  * @param  xend yend 结束坐标（不含）
  * @param  color     颜色
  * @retval None
  */

void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);//指定区域填充颜色
/**
  * @brief  用 DMA 刷一整块像素（LVGL 用这个）
  * @param  xsta ysta 起始坐标（含）
  * @param  xend yend 结束坐标（含）
  * @param  color     像素数组
  * @retval None
  */

void LCD_Color_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 *color);
/**
  * @brief  注册 DMA 刷屏完成的回调
  * @param  cb 回调函数
  * @retval None
  */

void LCD_Set_Flush_Complete_Callback(LCD_CallbackFunc_t cb);
/**
  * @brief  画一个点
  * @param  x y 坐标
  * @param  color 颜色
  * @retval None
  */

void LCD_DrawPoint(u16 x,u16 y,u16 color);//在指定位置画一个点
/**
  * @brief  画直线（Bresenham）
  * @param  x1 y1 起点
  * @param  x2 y2 终点
  * @param  color 颜色
  * @retval None
  */

void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);//在指定位置画一条线
/**
  * @brief  画矩形边框
  * @param  x1 y1 左上角
  * @param  x2 y2 右下角
  * @param  color 颜色
  * @retval None
  */

void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);//在指定位置画一个矩形
/**
  * @brief  画空心圆
  * @param  x0 y0 圆心
  * @param  r 半径
  * @param  color 颜色
  * @retval None
  */

void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color);//在指定位置画一个圆

/**
  * @brief  显示汉字串
  * @param  x y 左上角坐标
  * @param  s GBK 编码的汉字串
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号 12/16/24/32
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowChinese(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);//显示汉字串
/**
  * @brief  显示单个 12x12 汉字
  * @param  x y 左上角坐标
  * @param  s GBK 汉字
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowChinese12x12(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);//显示单个12x12汉字
/**
  * @brief  显示单个 16x16 汉字
  * @param  x y 左上角坐标
  * @param  s GBK 汉字
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowChinese16x16(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);//显示单个16x16汉字
/**
  * @brief  显示单个 24x24 汉字
  * @param  x y 左上角坐标
  * @param  s GBK 汉字
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowChinese24x24(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);//显示单个24x24汉字
/**
  * @brief  显示单个 32x32 汉字
  * @param  x y 左上角坐标
  * @param  s GBK 汉字
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowChinese32x32(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);//显示单个32x32汉字

/**
  * @brief  显示一个 ASCII 字符
  * @param  x y 左上角坐标
  * @param  num 字符
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode);//显示一个字符
/**
  * @brief  显示 ASCII 字符串
  * @param  x y 左上角坐标
  * @param  p 字符串
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode 0 非叠加 1 叠加
  * @retval None
  */

void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode);//显示字符串
/**
  * @brief  求 m 的 n 次方
  * @param  m 底数
  * @param  n 指数
  * @retval m^n
  */

u32 mypow(u8 m,u8 n);//求幂
/**
  * @brief  显示固定位数的整数，前导零用空格代替
  * @param  x y 左上角坐标
  * @param  num 整数
  * @param  len 总位数
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @retval None
  */

void LCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey);//显示整数变量
/**
  * @brief  显示带两位小数的浮点数
  * @param  x y 左上角坐标
  * @param  num 浮点数
  * @param  len 总位数（含小数点）
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @retval None
  */

void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey);//显示两位小数变量

/**
  * @brief  显示 RGB565 图片
  * @param  x y 左上角坐标
  * @param  length 宽
  * @param  width 高
  * @param  pic 图片数组
  * @retval None
  */

void LCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[]);//显示图片


//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE           	 0x001F
#define BRED             0XF81F
#define GRED 			       0XFFE0
#define GBLUE			       0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			     0XBC40 //棕色
#define BRRED 			     0XFC07 //棕红色
#define GRAY  			     0X8430 //灰色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色
#define GRAYBLUE       	 0X5458 //灰蓝色
#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			     0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

#endif





