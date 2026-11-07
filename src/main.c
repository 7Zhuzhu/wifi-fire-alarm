/* 基于 STC89C52 + ESP8266 的 WiFi 智能火灾报警系统
 * 说明：本工程由学习套件例程整理，用于个人学习记录；
 *      原例程中的第三方账号、服务器地址已替换为占位符，请自行申请后填入。
 */

#include <reg51.h>	         //调用单片机头文件
#define uchar unsigned char  //无符号字符型 宏定义	变量范围0~255
#define uint  unsigned int	 //无符号整型 宏定义	变量范围0~65535
#include <intrins.h>
#include "eeprom52.h"
#include"lcd1602.h"
#include "uart.h"
#include "esp8266.h"

uchar data_temp[16]= " Temp:    23.0 C";
uchar data_smoke[16]="Smoke:       40%";
sbit CS=P2^3;		//CS定义为P3口的第2位脚，连接ADC0832CS脚  PCB
sbit SCL=P2^0;		//SCL定义为P3口的第3位脚，连接ADC0832SCL脚
sbit DO=P2^1;		//DO定义为P3口的第4位脚，连接ADC0832DO脚

sbit ledy = P1^6;   //温度报警指示灯
sbit ledr = P1^7;   //烟雾报警指示灯 x
sbit dq   = P1^5;	//18b20 IO口的定义
sbit beep = P3^6;   //蜂鸣器IO口定义
uint temperature;
uint s_temp ;  //温度的变量
uchar dengji,s_dengji;     //烟物等级
           //手动报警键
unsigned char i=0,aa=0;
unsigned char flag=0;
unsigned char dat=0;
unsigned char flag_REC=0;
unsigned char buff[3]=0;
/* 上报帧缓冲区：原例程写 send[16]，但代码会写到 send[16]（阈值个位），
 * 属于越界写。这里扩到 18 字节留出余量。帧长实际 17 字节。 */
uchar send[18]="12340000888888";
bit flag_300ms = 1,shoudong=0;
uchar key_can=0;		 //按键值的变量
uchar menu_1=0;        //菜单设计的变量
uchar tt=0;

/***********************1ms延时函数*****************************/
void delay_1ms(uchar q)
{
	uchar i,j;
	for(i=0;i<q;i++)
		for(j=0;j<120;j++);
}

///***********************小延时函数*****************************/
//void delay_uint(uint q)
//{
//	while(q--);
//}


/******************把数据保存到单片机内部eeprom中******************/
void write_eeprom()
{
	SectorErase(0x2000);
	byte_write(0x2000, s_temp/256);
	byte_write(0x2001, s_temp%256);
	byte_write(0x2002, s_dengji);
	byte_write(0x2061, a_a);	
}

/******************把数据从单片机内部eeprom中读出来*****************/
void read_eeprom()
{
	s_temp   = byte_read(0x2000)*256+byte_read(0x2001);
	s_dengji = byte_read(0x2002);
	a_a      = byte_read(0x2061);
}

/**************开机自检eeprom初始化*****************/
void init_eeprom() 
{
	read_eeprom();		//先读
	if(a_a != 1)		//新的单片机初始单片机内问eeprom
	{
		s_temp   = 500;
		s_dengji = 50;
		a_a = 1;
		write_eeprom();	   //保存数据
	}	
}

/***********************18b20初始化函数*****************************/
void init_18b20()
{
	bit q;
	dq = 1;				//把总线拿高
	delay_uint(1);	    //15us
	dq = 0;				//给复位脉冲
	delay_uint(80);		//750us
	dq = 1;				//把总线拿高 等待
	delay_uint(10);		//110us
	q = dq;				//读取18b20初始化信号
	delay_uint(20);		//200us
	dq = 1;				//把总线拿高 释放总线
}

/*************写18b20内的数据***************/
void write_18b20(uchar dat)
{
	uchar i;
	for(i=0;i<8;i++)
	{					 //写数据是低位开始
		dq = 0;			 //把总线拿低写时间隙开始 
		dq = dat & 0x01; //向18b20总线写数据了
		delay_uint(5);	 // 60us
		dq = 1;			 //释放总线
		dat >>= 1;
	}	
}

/*************读取18b20内的数据***************/
uchar read_18b20()
{
	uchar i,value;
	for(i=0;i<8;i++)
	{
		dq = 1;			 //把总线拿低读时间隙开始 
		value >>= 1;	 //读数据是低位开始
		dq = 0;			 //释放总线
		if(dq == 1)		 //开始读写数据 
			value |= 0x80;
		delay_uint(5);	 //60us	读一个时间隙最少要保持60us的时间
	}
	return value;		 //返回数据
}

/*************读取温度的值 读出来的是小数***************/
uint read_temp()
{
	uint value;
	uchar low;			   //在读取温度的时候如果中断的太频繁了，就应该把中断给关了，否则会影响到18b20的时序
	
	init_18b20();		   //初始化18b20
	EA = 0;
	write_18b20(0xcc);	   //跳过64位ROM
	write_18b20(0x44);	   //启动一次温度转换命令
	EA = 1;
	delay_uint(50);		   //500us

	init_18b20();		   //初始化18b20
	
	EA = 0;
	write_18b20(0xcc);	   //跳过64位ROM
	write_18b20(0xbe);	   //发出读取暂存器命令
	EA = 1;
	
	EA = 0;
	low = read_18b20();	   //读温度低字节
	value = read_18b20();  //读温度高字节
	EA = 1;
	value <<= 8;		   //把温度的高位左移8位
	value |= low;		   //把读出的温度低位放到value的低八位中
	value *= 0.625;	       //转换到温度值 
	return value;		   //返回读出的温度 
}


/***********读数模转换数据********************************************************/	
//请先了解ADC0832模数转换的串行协议，再来读本函数，主要是对应时序图来理解，本函数是模拟0832的串行协议进行的

unsigned char ad0832read(bit SGL,bit ODD)
{
	unsigned char i=0,value=0,value1=0;		
		SCL=0;
		DO=1;
		CS=0;		//开始
		SCL=1;		//第一个上升沿	
		SCL=0;
		DO=SGL;
		SCL=1;  	//第二个上升沿
		SCL=1;
		DO=ODD;
		SCL=1;	    //第三个上升沿
		SCL=0;	    //第三个下降沿
		DO=1;
		for(i=0;i<8;i++)
		{
			SCL=0;
			SCL=1; //开始从第四个下降沿接收数据
			value<<=1;
			if(DO)
				value++;						
		}
		for(i=0;i<8;i++)
		{			//接收校验数据
			value1>>=1;
			if(DO)
				value1+=0x80;
			SCL=0;
			SCL=0;
		}
		CS=1;
		SCL=1;	
		if(value==value1)				//与校验数据比较，正确就返回数据，否则返回0	
			return value;
	return 0;
}




/*************定时器0初始化程序***************/
void time_init()	  
{
	EA   = 1;	 	  //开总中断
	TMOD = 0X21;	  //定时器0、定时器1工作方式1
	ET0  = 1;		  //开定时器0中断 
	TR0  = 1;		  //允许定时器0定时
}

/********************独立按键程序*****************/
uchar key_can;	 //按键值

void key()	 //独立按键程序
{
	static uchar key_new;
	key_can = 20;                   //按键值还原
	P1 |= 0x0f;
	if((P1 & 0x0f) != 0x0f)		//按键按下
	{
		delay_1ms(1);	     	//按键消抖动
		if(((P1 & 0x0f) != 0x0f) && (key_new == 1))
		{						//确认是按键按下
			key_new = 0;
			switch(P1 & 0x0f)
			{
				case 0x0e: key_can = 4; break;	   //得到k1键值
				case 0x0d: key_can = 3; break;	   //得到k2键值
				case 0x0b: key_can = 2; break;	   //得到k3键值
				case 0x07: key_can = 1; break;	   //得到k4键值
			}		
		}			
	}
	else 	   //按键松开
		key_new = 1;	
}

/****************按键处理数码管显示函数***************/
void key_with()
{
	static uchar  date_temp[16]="                ";
	static uchar date_smoke[16]="                ";
	if(key_can == 4) 	 //紧急报警键  手动报警
	{
		if(menu_1 == 0)
			shoudong = 1;
	}
	if(key_can == 1)	 //设置键
	{
		menu_1 ++;
		if(menu_1 >= 3)
		{
			menu_1 = 0;
		}
	}
	if(menu_1 == 0)
	{
		if((key_can == 2) || (key_can == 3))
			shoudong = 0;          //取消手动报警
	}
	if(menu_1 == 1)			//设置高温报警
	{
		if(key_can == 2)
		{
			s_temp +=10 ;		//高温报警值加1 
			if(s_temp > 999)
				s_temp = 999;
		}
		if(key_can == 3)
		{
			s_temp -=10 ;		//高温报警值减1 
			if(s_temp > 999)
				s_temp = 0 ;
		}
		date_temp[5]=s_temp/1000;
		if(date_temp[5]==0)
			 date_temp[5]=' ';
		else
			date_temp[5]+=0x30;
		date_temp[6]=s_temp/100%10+0x30;
		date_temp[7]=s_temp/10%10+0x30;
//		date_temp[8]='.';
//		date_temp[9]=s_temp%10+0x30;
		date_temp[8]=0XDF;
		date_temp[9]='C';
//		clear_1602();
		write_string(1,0,"   Temp_Alarm   ");
		write_string(2,0,date_temp);
		write_eeprom();			   //保存数据
	}	
	if(menu_1 == 2)			//设置烟物报警
	{
		if(key_can == 2)
		{
			s_dengji ++ ;	  //烟物报警值加1 
			if(s_dengji >= 99)
				s_dengji = 99;
		}
		if(key_can == 3)
		{
			s_dengji --;	  //烟物报警值减1 
			if(s_dengji <= 1)
				s_dengji = 1;
		}
//		clear_1602();
		date_smoke[7]=s_dengji/10+0x30;
		date_smoke[8]=s_dengji%10+0x30;
		date_smoke[9]='%';
		write_string(1,0,"  Smoke_Alarm   ");
		write_string(2,0,date_smoke);
		write_eeprom();			   //保存数据
	}	
}  
/****************报警函数***************/
void clock_h_l()
{
	static uchar value;
	if((dengji >= s_dengji) || (temperature >= s_temp) || (shoudong == 1))		//报警
	{
		value ++;
		if(value >= 2)
		{
			value = 10;
			beep = ~beep;	  //蜂鸣器报警
			if(dengji >= s_dengji)
			{
				ledr=~ledr;
				send[10]='0';
			}
			else
			{
				send[10]='1';
			}
			if(temperature >= s_temp)
			{
				ledy=~ledy;
				send[11]='0';
			}
			else
			{
				send[11]='1';
			}
			if(shoudong == 1)
			{
				ledy=~ledy;
				ledr=~ledr;
				send[12]='0';
			}
			else
			{
				send[12]='1';
			}
		}
	}else 
	{
		if((dengji < s_dengji) && (temperature < s_temp) && (shoudong == 0))	  //取消报警
		{
			value = 0;
			beep = 1;		 //取消报警
			ledr=1;//取消红灯报警
			ledy=1;//取消黄灯报警
			send[10]='1';
			send[11]='1';
			send[12]='1';
		}	
	}
}
void send_set()	 //接收设置报警值
{
	if(flag_REC==1)
	{
		flag_REC=0;
		if(buff[1]=='B')
		{
			switch (buff[2])
			{
				
				case '1': if(s_temp<1600)      s_temp+=10; if(s_temp>1600) s_temp=0;  break;
				case '2': if(s_temp>0 )       s_temp-=10;  break;
				case '3': if(s_dengji<100 )   s_dengji++;  break;
				case '4': if(s_dengji>0)      s_dengji--;  break;
				case '5': shoudong=~ shoudong; break;
			
			}
			write_eeprom();
		}
		
	}
}
/***************主函数*****************/
void main()
{
	
	beep = 0;				    //开机蜂鸣器叫一声
	delay_1ms(200);
	P0 = P1 = P2 = P3 = 0xff;   //初始化IO口为高电平
	init_eeprom();  //开始初始化保存的数据
	delay_1ms(200);	
	
				
	temperature = read_temp();  //读取温度值
	Uart_init();
	time_init(); //初始化定时器 
	
	init_1602();
	init_1602_dis_csf();
	Init_ESP8266();		
	while(1)  
	{
		key();					//独立按键程序
		send_set();
		if(key_can < 10)
		{
			key_with();			//按键按下要执行的程序
		}
		if(flag_300ms == 1)
		{		
			flag_300ms = 0;
			clock_h_l();
			temperature = read_temp();  //读取温度值	  
			if(temperature>1500)
				  temperature=0;
			dengji = ad0832read(1,0);	
			dengji = dengji * 100 / 250;
			if(dengji>100)
				dengji=0;
			data_temp[9]=temperature/1000;
			
			if(data_temp[9]==0)
			{
				data_temp[9]=' ';
				send[3]=' ';
			}
			else
			{
				data_temp[9]+=0x30;	
				send[3]=temperature/1000+0x30;
			}
			data_temp[10]=temperature/100%10;
			if(data_temp[10]==0)
			{
				data_temp[10]=' ';
				send[4]=' ';
			}
			else
			{
				data_temp[10]+=0x30;
				send[4]=temperature/100%10+0x30;	
			}
			data_temp[11]=temperature/10%10+0x30;
			data_temp[13]=temperature%10+0x30;
			data_temp[14]=0XDF;	  //°
			send[5]=temperature/10%10+0x30;
			send[6]='.';
			send[7]=temperature%10+0x30;

			data_smoke[13]=dengji/10+0x30;
			data_smoke[14]=dengji%10+0x30;

			send[8]=dengji/10+0x30;
			send[9]=dengji%10+0x30;
			send[13]=s_dengji/10+0x30;
			send[14]=s_dengji%10+0x30;
			send[15]=s_temp/100%10+0x30;
			send[16]=s_temp/10%10+0x30;
			if(menu_1 == 0)
			{
				if(temperature >= 999)
					temperature = 999;
				write_string(1,0,data_temp);	//显示温度
				write_string(2,0,data_smoke);	//显示烟雾浓度
			}
			aa++;
			if(aa>2)
			{
				aa=0;
				ESP8266_Register(send);
//				tt++;
//				if(tt>10)
//				{
//					tt=0;
//					SendString("AT+CIPSTART=\"TCP\",\"123.57.239.76\",10005\r\n");
//				}
			}
		}  
		
		delay_1ms(1);
	}
}

/*************定时器0中断服务程序***************/
void time0_int() interrupt 1
{	
	static uchar value;
	TH0 = 0xf8;
	TL0 = 0x30;     // 2ms
	value ++;	 
	if(value % 150 == 0)
	{
		flag_300ms = 1;	   //300ms
		value = 0;
	}
}
//  通讯中断接收程序   中断函数无返回值
  void uart_rx(void)  interrupt 4	using 3	 
 {

//   	tt=0;
	
	if(RI)	                 //是否接收中断
	{      
	RI=0;
	dat=SBUF;
	
	if(dat=='A'&&(i==0))
	{
		flag=1;        //开始接收数据
	}
	if(flag==1)
	{
		buff[i]=dat;
		i++;
		if(i>3)  // 停止接收
		{
			i=0;
			flag=0;
			flag_REC=1;

		}
	}
	
	}
 }