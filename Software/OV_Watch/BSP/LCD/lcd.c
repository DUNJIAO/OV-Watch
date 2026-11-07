#include "lcd.h"
#include "lcd_init.h"
#include "lcdfont.h"
#include "delay.h"
#include "spi.h"
#include "cmsis_os.h"

#define OFFSET_Y 20

static LCD_CallbackFunc_t lcd_ready_cb = NULL;

/**
  * @brief  在矩形区域内逐点填色
  * @param  xsta ysta 起始坐标（左上角，含）
  * @param  xend yend 结束坐标（右下角，不含）
  * @param  color     16 位 RGB565 颜色
  * @retval None
  * @note   起始行自动加上 OFFSET_Y；循环是 i<yend、j<xend，
  *         所以结束坐标是开区间，实际填充 (xend-xsta) x (yend-ysta) 个像素
  *         颜色是一个字节一个字节用 SPI 阻塞发送的，很慢，只用在上电欢迎页
  */

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{
	u16 i,j;
	LCD_Address_Set(xsta,ysta+OFFSET_Y,xend-1,yend-1+OFFSET_Y);//设置显示范围
	for(i=ysta;i<yend;i++)
	{
		for(j=xsta;j<xend;j++)
		{
			LCD_WR_DATA(color);
		}
	}
}

/**
  * @brief  用 DMA 把一整块像素刷到屏幕上（LVGL 用的就是这个）
  * @param  xsta ysta 起始坐标（含）
  * @param  xend yend 结束坐标（含）
  * @param  color_p   像素数据首地址，u16 数组，RGB565
  * @retval None
  * @note   结束坐标在这里是闭区间，和 LCD_Fill 的约定不一样；
  *         函数发完 DMA 立刻返回，真正写屏要等 HAL_SPI_TxCpltCallback 触发
  */

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Color_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 *color_p)
{
	u16 i,j,width,height;
	width = xend-xsta+1;
	height = yend-ysta+1;
	uint32_t pixel_count = width * height;
	uint32_t byte_count = pixel_count * 2U;

	LCD_Address_Set(xsta,ysta+OFFSET_Y,xend,yend+OFFSET_Y);

	HAL_SPI_Transmit_DMA(&hspi1,(uint8_t*)color_p,byte_count);
}

// 设置通知的回调函数
/**
  * @brief  注册"本次 DMA 刷屏完成"的回调
  * @param  cb 回调函数，NULL 表示不通知
  * @retval None
  * @note   lv_port_disp.c 用它把 LVGL 的 flush_ready 挂进来
  */

void LCD_Set_Flush_Complete_Callback(LCD_CallbackFunc_t cb)
{
	lcd_ready_cb = cb;
}

// DMA 传输完成中断回调
/**
  * @brief  HAL 的 SPI 发送完成中断回调
  * @param  hspi 触发回调的 SPI 句柄
  * @retval None
  * @note   先死等 BSY 清零，确保最后一个字节真的移出去了，再通知上层；
  *         在中断里忙等是坏习惯，但 ST7789 没有别的握手信号可用
  */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		// 确保最后一位数据离开 STM32 的移位寄存器
		while (hspi->Instance->SR & SPI_FLAG_BSY);
		if (lcd_ready_cb)
		{
			lcd_ready_cb();
		}
	}
}

/**
  * @brief  画一个点
  * @param  x y 坐标
  * @param  color 颜色
  * @retval None
  * @note   不加 OFFSET_Y，和 LCD_Fill / LCD_Color_Fill 不是同一套坐标系
  */

/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_Address_Set(x,y,x,y);//设置光标位置
	LCD_WR_DATA(color);
}


/**
  * @brief  任意两点之间画直线（Bresenham 算法）
  * @param  x1 y1 起点
  * @param  x2 y2 终点
  * @param  color 颜色
  * @retval None
  * @note   用一个累加误差 xerr/yerr 决定什么时候挪一格，全程只有整数运算；
  *         每画一个点都要重设一次窗口并单独写 2 字节，速度很慢
  */

/******************************************************************************
      函数说明：画线
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   线的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	u16 t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向
	else if (delta_x==0)incx=0;//垂直线
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线
	else {incy=-1;delta_y=-delta_y;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		LCD_DrawPoint(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}


/**
  * @brief  画矩形边框（四条线，不填充）
  * @param  x1 y1 左上角
  * @param  x2 y2 右下角
  * @param  color 颜色
  * @retval None
  */

/******************************************************************************
      函数说明：画矩形
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   矩形的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	LCD_DrawLine(x1,y1,x2,y1,color);
	LCD_DrawLine(x1,y1,x1,y2,color);
	LCD_DrawLine(x1,y2,x2,y2,color);
	LCD_DrawLine(x2,y1,x2,y2,color);
}


/**
  * @brief  画空心圆
  * @param  x0 y0 圆心
  * @param  r     半径
  * @param  color 颜色
  * @retval None
  * @note   每轮只算出第一个八分圆上的点，再按 x/y 互换和正负对称画出 8 个点；
  *         所以循环量只有半径的 0.707 倍左右
  */

/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
                color   圆的颜色
      返回值：  无
******************************************************************************/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	a=0;b=r;
	while(a<=b)
	{
		LCD_DrawPoint(x0-b,y0-a,color);             //3
		LCD_DrawPoint(x0+b,y0-a,color);             //0
		LCD_DrawPoint(x0-a,y0+b,color);             //1
		LCD_DrawPoint(x0-a,y0-b,color);             //2
		LCD_DrawPoint(x0+b,y0+a,color);             //4
		LCD_DrawPoint(x0+a,y0-b,color);             //5
		LCD_DrawPoint(x0+a,y0+b,color);             //6
		LCD_DrawPoint(x0-b,y0+a,color);             //7
		a++;
		if((a*a+b*b)>(r*r))//判断要画的点是否过远
		{
			b--;
		}
	}
}

/**
  * @brief  显示一串汉字
  * @param  x y   左上角坐标
  * @param  s     GBK 编码的汉字串
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号 12/16/24/32
  * @param  mode  0 非叠加（带背景色）1 叠加（只画前景点）
  * @retval None
  * @note   每次 s 前进 2 个字节，因为 GBK 一个汉字刚好两字节；
  *         x 前进 sizey，即每个字占 sizey x sizey 的方块
  */

/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示坐标
                *s 要显示的汉字串
                fc 字的颜色
                bc 字的背景色
                sizey 字号 可选 16 24 32
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	while(*s!=0)
	{
		if(sizey==12) LCD_ShowChinese12x12(x,y,s,fc,bc,sizey,mode);
		else if(sizey==16) LCD_ShowChinese16x16(x,y,s,fc,bc,sizey,mode);
		else if(sizey==24) LCD_ShowChinese24x24(x,y,s,fc,bc,sizey,mode);
		else if(sizey==32) LCD_ShowChinese32x32(x,y,s,fc,bc,sizey,mode);
		else return;
		s+=2;
		x+=sizey;
	}
}

/**
  * @brief  显示单个 12x12 汉字
  * @param  x y   左上角坐标
  * @param  s     GBK 汉字（前两字节）
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号，调用方保证是 12
  * @param  mode  0 非叠加 1 叠加
  * @retval None
  * @note   在 tfont12 表里按两字节编码逐个比对查出点阵；
  *         非叠加模式整块写满，叠加模式只画有笔画的位置
  */

/******************************************************************************
      函数说明：显示单个12x12汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese12x12(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;

	HZnum=sizeof(tfont12)/sizeof(typFNT_GB12);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont12[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont12[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}

/**
  * @brief  显示单个 16x16 汉字
  * @param  x y   左上角坐标
  * @param  s     GBK 汉字（前两字节）
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号，调用方保证是 16
  * @param  mode  0 非叠加 1 叠加
  * @retval None
  */

/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese16x16(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
  TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont16[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont16[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}


/**
  * @brief  显示单个 24x24 汉字
  * @param  x y   左上角坐标
  * @param  s     GBK 汉字（前两字节）
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号，调用方保证是 24
  * @param  mode  0 非叠加 1 叠加
  * @retval None
  */

/******************************************************************************
      函数说明：显示单个24x24汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese24x24(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont24[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont24[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}

/**
  * @brief  显示单个 32x32 汉字
  * @param  x y   左上角坐标
  * @param  s     GBK 汉字（前两字节）
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号，调用方保证是 32
  * @param  mode  0 非叠加 1 叠加
  * @retval None
  */

/******************************************************************************
      函数说明：显示单个32x32汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese32x32(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont32[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont32[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}


/**
  * @brief  显示一个 ASCII 字符
  * @param  x y   左上角坐标
  * @param  num   字符，例如 'A'
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号 12/16/24/32，宽度固定是字号的一半
  * @param  mode  0 非叠加 1 叠加
  * @retval None
  * @note   num-' ' 是把 ASCII 码换算成字库数组的下标，字库里从空格开始存；
  *         点阵按行扫描，一个字节管 8 个横点
  */

/******************************************************************************
      函数说明：显示单个字符
      入口数据：x,y显示坐标
                num 要显示的字符
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 temp,sizex,t,m=0;
	u16 i,TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	sizex=sizey/2;
	TypefaceNum=(sizex/8+((sizex%8)?1:0))*sizey;
	num=num-' ';    //得到偏移后的值
	LCD_Address_Set(x,y,x+sizex-1,y+sizey-1);  //设置光标位置
	for(i=0;i<TypefaceNum;i++)
	{
		if(sizey==12)temp=ascii_1206[num][i];		       //调用6x12字体
		else if(sizey==16)temp=ascii_1608[num][i];		 //调用8x16字体
		else if(sizey==24)temp=ascii_2412[num][i];		 //调用12x24字体
		else if(sizey==32)temp=ascii_3216[num][i];		 //调用16x32字体
		else return;
		for(t=0;t<8;t++)
		{
			if(!mode)//非叠加模式
			{
				if(temp&(0x01<<t))LCD_WR_DATA(fc);
				else LCD_WR_DATA(bc);
				m++;
				if(m%sizex==0)
				{
					m=0;
					break;
				}
			}
			else//叠加模式
			{
				if(temp&(0x01<<t))LCD_DrawPoint(x,y,fc);//画一个点
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y++;
					break;
				}
			}
		}
	}
}


/**
  * @brief  显示一个以 0 结尾的 ASCII 字符串
  * @param  x y   左上角坐标
  * @param  p     字符串
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @param  mode  0 非叠加 1 叠加
  * @retval None
  * @note   每画一个字符 x 前进 sizey/2，因为 ASCII 字符宽是字号的一半
  */

/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y显示坐标
                *p 要显示的字符串
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	while(*p!='\0')
	{
		LCD_ShowChar(x,y,*p,fc,bc,sizey,mode);
		x+=sizey/2;
		p++;
	}
}


/**
  * @brief  求 m 的 n 次方（整数版 pow）
  * @param  m 底数
  * @param  n 指数
  * @retval m^n
  * @note   n 是 u8，n-- 到 0 会回绕成 255，所以调用前必须保证 n>0
  */

/******************************************************************************
      函数说明：显示数字
      入口数据：m底数，n指数
      返回值：  无
******************************************************************************/
u32 mypow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;
	return result;
}


/**
  * @brief  显示固定位数的整数，前面用空格补齐
  * @param  x y   左上角坐标
  * @param  num   要显示的整数
  * @param  len   总位数
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @retval None
  * @note   enshow 标志实现"去掉前导零"：遇到第一个非零数字之前都画空格；
  *         但 len-1 这一位无论如何都会画出来，所以 0 也能显示成 "0"
  */

/******************************************************************************
      函数说明：显示整数变量
      入口数据：x,y显示坐标
                num 要显示整数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey)
{
	u8 t,temp;
	u8 enshow=0;
	u8 sizex=sizey/2;
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+t*sizex,y,' ',fc,bc,sizey,0);
				continue;
			}else enshow=1;

		}
	 	LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
}


/**
  * @brief  显示带两位小数的浮点数
  * @param  x y   左上角坐标
  * @param  num   要显示的浮点数
  * @param  len   总位数（含小数点）
  * @param  fc bc 前景色 / 背景色
  * @param  sizey 字号
  * @retval None
  * @note   num*100 转成整数再逐位取数字，小数部分到 len-2 位时插入小数点并把 len 加一；
  *         所以 len 必须是 >=3 的奇数，否则小数点位置会错
  */

/******************************************************************************
      函数说明：显示两位小数变量
      入口数据：x,y显示坐标
                num 要显示小数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey)
{
	u8 t,temp,sizex;
	u16 num1;
	sizex=sizey/2;
	num1=num*100;
	for(t=0;t<len;t++)
	{
		temp=(num1/mypow(10,len-t-1))%10;
		if(t==(len-2))
		{
			LCD_ShowChar(x+(len-2)*sizex,y,'.',fc,bc,sizey,0);
			t++;
			len+=1;
		}
	 	LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
}


/**
  * @brief  显示一张 RGB565 图片
  * @param  x y        左上角坐标
  * @param  length     宽（像素）
  * @param  width      高（像素）
  * @param  pic        图片数组，每像素两字节，高字节在前
  * @retval None
  * @note   逐字节用 LCD_WR_DATA8 发送，没有用 DMA，大图会明显可见刷屏过程
  */

/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标
                length 图片长度
                width  图片宽度
                pic[]  图片数组
      返回值：  无
******************************************************************************/
void LCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[])
{
	u16 i,j;
	u32 k=0;
	LCD_Address_Set(x,y,x+length-1,y+width-1);
	for(i=0;i<length;i++)
	{
		for(j=0;j<width;j++)
		{
			LCD_WR_DATA8(pic[k*2]);
			LCD_WR_DATA8(pic[k*2+1]);
			k++;
		}
	}
}


