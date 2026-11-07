#ifndef _UART_H_
#define	 _UART_H_

//#include"reg52.h"

#define uchar unsigned char
#define uint unsigned int

  

void delay(uint xms)//延时函数，有参函数
{
	uint x, y;
	for(x=xms;x>0;x--)
	for(y=110;y>0;y--);
}  
void Uart_init()
{

	TH0=0xff;
	TL0=0x48;		//T0赋初值25us
 
	ET0=1;		   //打开中断允许开关
	TR0=0;		   //打开定时器开关
	TMOD= 0X21;		//T1  方式2  ，8位   自动重装   
	TH1=0Xfd;
	TL1=0Xfd;	    //9600
	TR1=1; 		    // 定时器1启动  
	SM0=0;	    // 设置串口的工作模式
	SM1=1; 	    //方式1
	REN=1; 		 // 允许串口接收数据 	 
	ES=1; 	     // 串口中断应许 
	EA=1;		 // 开启中断	
}
 void SendASC(uchar d)
{
	
	SBUF=d;
	while(!TI);

	TI=0;
}
void SendString(uchar *str)	
{
	ES=0;
	while(*str)
	{
		SendASC(*str) ;
		str++;
	}
	ES=1;
}


#endif
			