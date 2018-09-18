/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           UDP_App.c
** Last modified Date:  2015-11-14
** Last Version:        v1.0
** Description:         UDP以太网串口数据转换模实现
**
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo任海波
** Created date:        2015-11-14
** Version:             v1.0
** Descriptions:        The original version 初始版本
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
**
*********************************************************************************************************/
#include <includes.h>

/*********************************************************************************************************
** Function name:       udp_app_thread
** Descriptions:        udp例子程序
** input parameters:    *p_arg
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
static uint8_t udp_buffer[1024];
void udp_app_thread(void *parg)
{
  int s;
  int iMode = 1;                            //  0：阻塞
  int len;
  uint32_t recvLen;
  struct sockaddr_in myAddr,destAddr,recvAddr;

  // 设置本地IP地址及端口号
  myAddr.sin_family = AF_INET;
  myAddr.sin_port = htons(8000);
  myAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  // 设置远程IP地址及端口号
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(8000);
  destAddr.sin_addr.s_addr = inet_addr("192.168.2.25");

  recvAddr = destAddr;

  recvLen = sizeof(recvAddr);

  s = socket(PF_INET,SOCK_DGRAM,0);
  //ioctlsocket(s,FIONBIO,&iMode);            //非阻塞设置
  bind(s,(const struct sockaddr *)&myAddr,sizeof(myAddr));

  for(;;)
  {
     len = recvfrom(s, udp_buffer, 1024, 0, (struct sockaddr *)&recvAddr,&recvLen);
     if(len > 0) {
        sendto(s, udp_buffer,len,0,
                 ( const struct sockaddr *)&recvAddr,sizeof(recvAddr));
     }
  }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
int cmd_udp_test(int argc, char **argv)
{
    rt_device_t tid;

    if (argc == 1)
    {
        tid = rt_thread_create("udp", udp_app_thread, RT_NULL, 1024, 10, 20);
        if (tid != RT_NULL) {
            rt_thread_startup(tid);
            rt_kprintf("create udp test thread ok!\r\n");
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_udp_test, __cmd_udp_test, create udp example thread);
#endif
/*********************************************************************************************************
** End of File
*********************************************************************************************************/
