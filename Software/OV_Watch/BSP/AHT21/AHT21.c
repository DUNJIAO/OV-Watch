#include "AHT21.h"

/* 开启 AHT21 所在端口 GPIOB 的时钟 */
#define AHT_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()

/**
  * @brief  AHT21 的总线定义：SDA = PB13，SCL = PB14
  * @note   7 位器件地址为 0x38，线上出现的是 0x70（写）/ 0x71（读）
  */
iic_bus_t AHT_bus =
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/**
  * @brief  读取器件状态字节
  * @note   整个过程只有一次起始、一次停止：直接寻址 0x71（读）
  * @retval 状态字节：bit7 = 1 表示正在测量，bit3 = 1 表示已标定
  */
uint8_t AHT_Read_Status(void)
{
	uint8_t Byte_first;
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x71);
	IICWaitAck(&AHT_bus);
	Byte_first = IICReceiveByte(&AHT_bus);
	IICSendNotAck(&AHT_bus);
	IICStop(&AHT_bus);
	return Byte_first;
}

/**
  * @brief  判断器件是否已标定（NOR 模式）
  * @note   头文件里没有声明，外部无法调用；本工程中也没有任何调用点。
  *         判断依据是状态字节：屏蔽掉无关位后等于 0x08，说明 bit3 = 1
  * @retval 1：已标定（bit3 = 1）；0：未标定（bit3 = 0）
  */
uint8_t AHT_Read_Cal_Enable(void)  //check cal enable bit
{
	uint8_t val = 0;//ret = 0,

  val = AHT_Read_Status();
  if((val & 0x68)==0x08)  //check NOR mode
		return 1;
  else
		return 0;
}

/**
  * @brief  软复位，向器件发送命令 0xBA
  * @note   先写器件地址 0x70，再写单字节命令，没有寄存器地址
  * @retval None
  */
void AHT_Reset(void)//AHT21 send 0xBA reset call
{
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x70);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0xBA);
	IICWaitAck(&AHT_bus);
	IICStop(&AHT_bus);
}

/**
  * @brief  初始化：开启 GPIOB 时钟并初始化软件 I2C
  * @note   标定：器件出厂时逐个测过，各自的补偿参数存在芯片内部。
  *         状态字节 bit3 = 1 表示参数已加载，读数可信；
  *         bit3 = 0 表示未加载，要先发 0xBE 0x08 0x00 让芯片把参数读出来，
  *         再等约 10 ms，之后读数才准。
  *         本函数本意是"未标定就发这条初始化命令"，
  *         但第 84 行条件误写成 &&（应为 &），该段实际永远不会执行。
  * @retval 固定为 0，不代表成败
  */
uint8_t AHT_Init(void)
{
	AHT_CLK_ENABLE;
	IICInit(&AHT_bus);

	delay_ms(40);

	if(AHT_Read_Status()&&0x08!=0x08)
	{
		IICStart(&AHT_bus);
		IICSendByte(&AHT_bus,0x70);
		IICWaitAck(&AHT_bus);
		IICSendByte(&AHT_bus,0xBE);
		IICWaitAck(&AHT_bus);
		IICSendByte(&AHT_bus,0x08);
		IICWaitAck(&AHT_bus);
		IICSendByte(&AHT_bus,0x00);
		IICWaitAck(&AHT_bus);
		IICStop(&AHT_bus);
		delay_ms(10);
	}
	//AHT_Reset();

	return 0;
}

/**
  * @brief  触发一次测量并读取结果
  * @param  humi  输出参数，湿度，单位 %RH
  * @param  temp  输出参数，温度，单位 摄氏度
  * @note   1) 发 0xAC 0x33 0x00 触发测量
  *         2) 等约 80 ms 后轮询状态位，最多再等 5 × 5 ms
  *         3) 连续读 6 个字节，前 5 个回 ACK，最后一个回 NACK
  *         4) 前 20 bit 是湿度，后 20 bit 是温度
  * @retval 0：成功；1：等待测量完成超时
  */
uint8_t AHT_Read(float *humi, float *temp)
{
	uint8_t cnt=5;
	uint8_t  Byte_1th=0;
	uint8_t  Byte_2th=0;
	uint8_t  Byte_3th=0;
	uint8_t  Byte_4th=0;
	uint8_t  Byte_5th=0;
	uint8_t  Byte_6th=0;
	uint32_t RetuData = 0;

	/* 第一步：发送触发测量命令 */
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x70);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0xAC);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0x33);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0x00);
	IICWaitAck(&AHT_bus);
	IICStop(&AHT_bus);

	/* 第二步：等转换完成，最多 5 次 × 5 ms，超时返回 1 */
	delay_ms(80);
	while(AHT_Read_Status()&0x80==0x80 && cnt)
	{
		delay_ms(5);
		cnt--;
		AHT_Read_Status();
	}
	if(!cnt)
	{return 1;}

	/* 第三步：连续读 6 个字节 */
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x71);
	IICWaitAck(&AHT_bus);
	Byte_1th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_2th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_3th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_4th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_5th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_6th = IICReceiveByte(&AHT_bus);
	IICSendNotAck(&AHT_bus);
	IICStop(&AHT_bus);

	/* 第四步：拼数据。Byte_1 是状态，湿度 = Byte_2~4 共 20 bit */
	RetuData = (RetuData|Byte_2th)<<8;
	RetuData = (RetuData|Byte_3th)<<8;
	RetuData = (RetuData|Byte_4th);
	RetuData =RetuData >>4;
	*humi = (RetuData * 1000 >> 20);
	*humi /= 10;

	/* 温度 = Byte_4 低 4 位 + Byte_5 + Byte_6，共 20 bit */
	RetuData = 0;
	RetuData = (RetuData|(Byte_4th&0x0f))<<8;
	RetuData = (RetuData|Byte_5th)<<8;
	RetuData = (RetuData|Byte_6th);
	RetuData = RetuData&0xfffff;
	*temp = ((RetuData * 2000 >> 20)- 500);
	*temp /= 10;

	return 0;
}
