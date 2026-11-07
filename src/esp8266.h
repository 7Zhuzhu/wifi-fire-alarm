/* ============================================================
 * ESP8266 AT 指令驱动（STC89C52 @11.0592MHz，9600bps）
 *
 * 说明：本文件由学习套件例程整理。原例程中写死的他人账号、
 *      服务器 IP、路由器名与密码已全部替换为占位符，
 *      使用前请替换为你自己的 SSID / 密码 / 服务器地址。
 * ============================================================ */
#define UN "your_email@example.com"       // 请替换为你在所用平台注册的账号
#define VER "A1.1"                        // 开发板版本号
#define ID "00000"                        // 请替换为你的开发板 ID
#define MAC "000000000000"                // 请替换为你的模块 MAC
#define RET 1                             // 是否请求返回控制数据

// 服务器参数
#define SERVER_MODE "TCP"

 unsigned char 	TempData;
 unsigned char 	HumiData;
 unsigned char 	SocData;
 void CodeJsonDataAndSend(unsigned char tempData, unsigned char tempHMData, unsigned char tempSocData);

/* 连接路由器 + 建立 TCP 连接
 * 注意：这里写的是占位符，替换成自己的 SSID/密码/服务器地址后再烧录。
 *      AT 指令里双引号与反斜杠都不能删。 */
void Init_ESP8266(void)
{
	SendString("AT+CWJAP=\"SSID\",\"PASSWORD\"\r\n");   // ← 换成你的 WiFi 名称与密码
	delay(6);
	SendString("AT+CIPSTART=\"TCP\",\"0.0.0.0\",0000\r\n");  // ← 换成你的服务器 IP 与端口
	delay(1);
}

/* 上报一帧数据
 * 修复点：原例程 AT+CIPSEND=20 但 send 缓冲区只有 16 字节，
 *        循环发 20 字节会读到缓冲区外的内存，手机端表现为"数据偶尔乱码"。
 *        这里改为按实际帧长 17 字节发送。 */
void ESP8266_Register(uchar *da)
{
	uchar *str2="AT+CIPSEND=17\r\n";
	uchar i;
	SendString(str2);
	delay(1);
	for(i=0;i<17;i++)
	{
		SendASC(da[i]);
	}
	delay(1);
}