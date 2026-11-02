#include "HrAlgorythm.h"
#include "string.h"

Queue datas;
Queue times;
Queue HR_List;

void HR_AlgoInit(void)
{
	initQueue(&datas);
	initQueue(&times);
	initQueue(&HR_List);
}

/**
 * @brief  心率均值滤波
 * @param  HrList:心率数据列表， lenth:数据长度
 * @retval 滤波后的平均心率
 */
uint8_t Hr_Ave_Filter(uint32_t *HrList, uint8_t lenth)
{
	uint32_t ave = 0;
	uint8_t i;
	for(i = 0;i<lenth;i++)
	{
		ave += HrList[i];
	}
	ave /= lenth;
	return ave;
}

/**
 * @brief  计算心率
 * @param  present_dat:第一站读到的原始数据， present_time:第一站读到的时间
 * @retval 心率值，单位：bpm
 */
uint16_t HR_Calculate(uint16_t present_dat,uint32_t present_time)
{

	static uint16_t peaks_time[]={0,0}; //记住最近两次的波峰时间
	static uint8_t HR=0; //上一次算出的心率

	if(isQueueFull(&datas))
	{dequeue(&datas);} //队列满了，就踢掉最老的
	if(isQueueFull(&times))
	{dequeue(&times);}
	if(isQueueFull(&HR_List))
	{dequeue(&HR_List);}

	enqueue(&datas,present_dat); //把这次的原始数据放入队列
	enqueue(&times,present_time); //把这次的时间放入队列

	if((datas.data[3]>=datas.data[2]) && (datas.data[3]>=datas.data[1]) && (datas.data[3]>datas.data[0]) 
		&& (datas.data[3]>=datas.data[4]) && (datas.data[3]>=datas.data[5]) && (datas.data[3]>datas.data[6]))
	{
			if((times.data[3]-peaks_time[0])>425)
			{
					peaks_time[1] = peaks_time[0];
					peaks_time[0] = times.data[3];
					enqueue(&HR_List,60000/(peaks_time[0]-peaks_time[1]));
					if(HR_List.data[6]!=0)
					{HR = Hr_Ave_Filter(HR_List.data,7);}
			}
	}
	return HR;
}



