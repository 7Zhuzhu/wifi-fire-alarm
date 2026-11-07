#ifndef _LCD1602_H_
#define _LCD1602_H_

#define uchar unsigned char 
#define uint unsigned int 

#define data_1602 P0

sbit rs=P2^5;	 //寄存器选择信号 H:数据寄存器  	L:指令寄存器
sbit rw=P2^6;	 //寄存器选择信号 H:数据寄存器  	L:指令寄存器
sbit e =P2^7;	 //片选信号   下降沿触发

/***********************延时函数************************/
void delay_uint(uchar q)
{
	while(q--);
}

/***********************lcd1602写命令函数************************/
void write_com(uchar com)
{
	e=0;
	rs=0;
	rw=0;
	P0=com;
	delay_uint(3);
	e=1;
	delay_uint(25);
	e=0;
}

/***********************lcd1602写数据函数************************/
void write_data(uchar dat)
{
	e=0;
	rs=1;
	rw=0;
	P0=dat;
	delay_uint(3);
	e=1;
	delay_uint(25);
	e=0;	
}

/***********************lcd1602初始化设置************************/
void init_1602()
{
	write_com(0x38);	//
	write_com(0x0c);
	write_com(0x06);
}







/***********************lcd1602上显示这字符函数************************/
void write_string(uchar hang,uchar add,uchar *p)
{
	uchar i;
	if(hang==1)   
		write_com(0x80+add);
	else
		write_com(0x80+0x40+add);
		for(i=0;i<16;i++)
		{
			write_data(*p);
			p++;
		}	
}


/****************开机液晶显示函数 初始化液晶的内容********************************/
void init_1602_dis_csf()
{					  
	write_string(1,0,"  WiFi init.... ");	
	write_string(2,0,"                ");				
}

#endif