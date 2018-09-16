/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           lwip_pppos.h
** Last modified Date:  2017-11-02
** Last Version:        v1.00
** Description:         lwipͨ��GPRS���ŵ�PPPOSӦ��
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2017-11-02
** Version:             v1.00
** Descriptions:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Description:         
*********************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/serial.h>
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppoe.h"
#include "netif/ppp/pppol2tp.h"

#include <lwip_if.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>

#include "board_serial.h"

#ifdef RT_USE_PPPOS

/*********************************************************************************************************
**  ���Ŵ�������
*********************************************************************************************************/
#define AT_TRY_NUM               20
#define POWER_TRY_NUM            20
#define PPP_TRY_NUM              20

/*********************************************************************************************************
**  ���������ṹ
*********************************************************************************************************/
typedef struct {
  ppp_pcb *ppp;
  rt_device_t serial;
  rt_uint16_t try_connect_counter;
  struct netif netif;
  rt_uint8_t buffer[1024];
}pppos_device;

static char __gCCID[32] = {0};
static rt_uint16_t __gRSSI;
/*********************************************************************************************************
**  ����ȫ�ֱ���
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       at_command
** Descriptions:        ATָ���
** input parameters:    handle: ʹ�õĴ��ھ��
*                       buffer: ��������
**                      at��    AT����
**                      ack��   Ӧ���־
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
int at_command(rt_device_t handle, char * buffer, const char *at, const char *ack)
{
  rt_uint32_t u32Start;
  rt_int32_t  u32Len;
  
  sprintf(buffer, "%s", at);
  rt_device_write(handle, 0, buffer, strlen(buffer));
  u32Len = rt_device_read(handle, 0, buffer, 256);
  buffer[u32Len] = 0;
  if(u32Len < 2) {
    u32Start = u32Len;
    u32Len = 256 - u32Start;
    u32Len = rt_device_read(handle, 0, &buffer[u32Start], u32Len);
    u32Start += u32Len;
    buffer[u32Start] = 0;
  }
  
  if(strstr(&buffer[2], "\r\n") == RT_NULL) {
    u32Len = 256 - u32Start;
    u32Len = rt_device_read(handle, 0, &buffer[u32Start], u32Len);
    u32Start += u32Len;
    buffer[u32Start] = 0;
    if(strstr(&buffer[2], "\r\n") == RT_NULL) {
      u32Len = 256 - u32Start;
      u32Len = rt_device_read(handle, 0, &buffer[u32Start], u32Len);
      u32Start += u32Len;
      buffer[u32Start] = 0;
      if(strstr(&buffer[2], "\r\n") == RT_NULL) {
        u32Len = 256 - u32Start;
        u32Len = rt_device_read(handle, 0, &buffer[u32Start], u32Len);
        u32Start += u32Len;
        buffer[u32Start] = 0;
        u32Len = 256 - u32Start;
      }
    }
  }
  
  if(strstr(buffer, ack) == RT_NULL) {
    return -1;
  }
  
  return 0;
}

/*********************************************************************************************************
** Function name:       pppos_status_cb
** Descriptions:        PPP status callback
** input parameters:    netif:	����ӿڱ�־
**                      p:      �������ݻ�����
** output parameters:   ��
** Returned value:      ���ͽ��
*********************************************************************************************************/
static void pppos_status_cb(ppp_pcb *pcb, int err_code, void *ctx) 
{
  struct netif *pppif = ppp_netif(pcb);
  pppos_device *device = ctx;
  
  LWIP_UNUSED_ARG(ctx);
  
  pppif = pppif;

  switch(err_code) {
    case PPPERR_NONE: {
#if LWIP_DNS
      const ip_addr_t *ns;
#endif /* LWIP_DNS */
      rt_kprintf("status_cb: Connected\n");
#if PPP_IPV4_SUPPORT
      rt_kprintf("   our_ipaddr  = %s\n", ipaddr_ntoa(&pppif->ip_addr));
      rt_kprintf("   his_ipaddr  = %s\n", ipaddr_ntoa(&pppif->gw));
      rt_kprintf("   netmask     = %s\n", ipaddr_ntoa(&pppif->netmask));
#if LWIP_DNS
      ns = dns_getserver(0);
      ns = ns;
      rt_kprintf("   dns1        = %s\n", ipaddr_ntoa(ns));
      ns = dns_getserver(1);
      rt_kprintf("   dns2        = %s\n", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
      rt_kprintf("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
      break;
    }
    case PPPERR_PARAM: {
      rt_kprintf("status_cb: Invalid parameter\n");
      break;
    }
    case PPPERR_OPEN: {
      rt_kprintf("status_cb: Unable to open PPP session\n");
      break;
    }
    case PPPERR_DEVICE: {
      rt_kprintf("status_cb: Invalid I/O device for PPP\n");
      break;
    }
    case PPPERR_ALLOC: {
      rt_kprintf("status_cb: Unable to allocate resources\n");
      break;
    }
    case PPPERR_USER: {
      rt_kprintf("status_cb: User interrupt\n");
      break;
    }
    case PPPERR_CONNECT: {
      rt_kprintf("status_cb: Connection lost\n");
      break;
    }
    case PPPERR_AUTHFAIL: {
      rt_kprintf("status_cb: Failed authentication challenge\n");
      break;
    }
    case PPPERR_PROTOCOL: {
      rt_kprintf("status_cb: Failed to meet protocol\n");
      break;
    }
    case PPPERR_PEERDEAD: {
      rt_kprintf("status_cb: Connection timeout\n");
      break;
    }
    case PPPERR_IDLETIMEOUT: {
      rt_kprintf("status_cb: Idle Timeout\n");
      break;
    }
    case PPPERR_CONNECTTIME: {
      rt_kprintf("status_cb: Max connect time reached\n");
      break;
    }
    case PPPERR_LOOPBACK: {
      rt_kprintf("status_cb: Loopback detected\n");
      break;
    }
    default: {
      rt_kprintf("status_cb: Unknown error code %d\n", err_code);
      break;
    }
  }

/*
 * This should be in the switch case, this is put outside of the switch
 * case for example readability.
 */

  if (err_code == PPPERR_NONE) {
    device->try_connect_counter = 0;
    return;
  }

  /* ppp_close() was previously called, don't reconnect */
  if (err_code == PPPERR_USER) {
    /* ppp_free(); -- can be called here */
    return;
  }

  /*
   * Try to reconnect in 30 seconds, if you need a modem chatscript you have
   * to do a much better signaling here ;-)
   */
  ppp_connect(pcb, 30);
  device->try_connect_counter++;
  if(device->try_connect_counter > PPP_TRY_NUM) {
    // ֱ�Ӹ�λϵͳ
    rt_hw_cpu_reset();
  }
  /* OR ppp_listen(pcb); */
}

/*********************************************************************************************************
** Function name:       pppos_output_cb
** Descriptions:        ppp�������ݺ���
** input parameters:    pcb:	ppp�ӿڱ�־
**                      data:   �������ݻ�����
**                      len��   �������ݳ��� 
**                      ctx��   ����������
** output parameters:   ��
** Returned value:      ʵ�ʷ������ݳ���
*********************************************************************************************************/
static u32_t pppos_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) 
{
  pppos_device *device = ctx;
  
  return rt_device_write(device->serial, 0, data, len);
}

/*********************************************************************************************************
** Function name:       pppos_rx_thread
** Descriptions:        ppp���������߳�
** input parameters:    netif:	����ӿڱ�־
**                      p:      �������ݻ�����
** output parameters:   ��
** Returned value:      ���ͽ��
*********************************************************************************************************/
static void pppos_rx_thread(void *parg)
{
  char *buffer;
  u16_t holdoff = 0;
  rt_uint32_t len;
  rt_uint16_t at_try_count;
  rt_uint16_t power__try_count;
  pppos_device *device = parg;
  rt_device_t handle = device->serial;
  
  buffer = rt_malloc(256);
  if(buffer == RT_NULL) {
    rt_kprintf("pppos thread can't malloc memory\r\n");
    return;
  }
  
  /*
  ** Step 1, �ȴ�CPU�������
  */
  rt_thread_delay(RT_TICK_PER_SECOND * 8);
  
  power__try_count = 0;
Power_Start_Again:
  power__try_count++;
  if(power__try_count > POWER_TRY_NUM) {
   // ֱ�Ӹ�λϵͳ
    rt_hw_cpu_reset();
  }
  /*
  ** Step 2, ���ô��ڽ��գ��볬ʱ
  */
  len = 3000;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &len);

  /*
  ** Step 3, �ȴ�ģ���ϵ���ɣ���������ϵ緢�͵�����
  */
  rt_thread_delay(RT_TICK_PER_SECOND * 8);
  while(rt_device_read(handle, 0, buffer, 256) > 0);
  rt_thread_delay(RT_TICK_PER_SECOND);
  
  /*
  ** Step 4, ��ȡģ��̼��汾��Ϣ
  */
//  memset(buffer, 0 ,256);
//  at_command(handle,buffer, "AT+GMR\r\n", "OK")
  
  /*
  ** Step 5, ��֤ATָ���Ƿ��ܹ���Ӧ
  */
  at_try_count = 0;
AT_Config_Start:
  at_try_count++;
  if(at_try_count > AT_TRY_NUM) {
    goto Power_Start_Again;
  }
  len = 3000;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &len);
  rt_thread_delay(RT_TICK_PER_SECOND * 2);
  while(rt_device_read(handle, 0, buffer, 256) > 0);
  rt_thread_delay(RT_TICK_PER_SECOND);
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 6, �رջ���
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "ATE0\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 7, ���SIM���Ƿ�Ready
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CPIN?\r\n", "READY") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 8, ��ѯ����CCID
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CCID\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  //��ȡCCID
  {
    char *pStart;
    char *pEnd;
    pStart = strstr(buffer, "+CCID:");
    if(pStart != RT_NULL) {
       pStart += 7;
       pEnd = strstr(pStart, "\r\n");
       if(pEnd != RT_NULL) {
         *pEnd  = '\0';
         memset(__gCCID, 0, 32);
         strcpy(__gCCID, pStart);
       }
    }
  }
  
  /*
  ** Step 9, ���ģ���Ƿ�ע������
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CREG?\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 10, �رյ绰Ӧ��ʽ
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "ATS0=0\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 11, �������ӵ�GPRS����
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CGATT=1\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  rt_thread_delay(RT_TICK_PER_SECOND * 3);
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CGREG=?\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 12, �鿴GPRS���������Ϣ
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CGDCONT=?\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  
  /*
  ** Step 12-2, ��ѯ�ź�ǿ��
  */
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CSQ\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
  //��ȡRSSI
  {
    char *pStart;
    char *pEnd;
    pStart = strstr(buffer, "+CSQ:");
    if(pStart != RT_NULL) {
      pStart += 6;
      pEnd = strstr(pStart, ",");
      if(pEnd != RT_NULL) {
        *pEnd  = '\0';
        __gRSSI = atoi(pStart);
      }
    }
  }
  
  /*
  ** Step 13,  Before active, use this command to set PDP context.�˺��������ܵȴ��ϳ�
  */
  len = 100000;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &len);
  memset(buffer, 0 ,256);
  if(at_command(handle,buffer, "AT+CGDCONT=1, \"IP\", \"CMNET\"\r\n", "OK") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }
 
 // rt_thread_delay(RT_TICK_PER_SECOND * 3);
//  /*
//  ** Step 14, Active command is used to active the specified PDP context.
//  */
//  len = 100000;
//  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &len);
//  memset(buffer, 0 ,256);
//  if(at_command(handle,buffer, "AT+CGACT=1,1\r\n", "OK") < 0) {
//    rt_thread_delay(RT_TICK_PER_SECOND * 3);
//    goto AT_Config_Start;
//  }
  
//  rt_thread_delay(RT_TICK_PER_SECOND * 3);
  /*
  ** Step 15, This command is to start PPP translation.
  */
  memset(buffer, 0 ,256);
//  if(at_command(handle,buffer, "ATD*99***1#\r\n", "CONNECT") < 0) {
//    rt_thread_delay(RT_TICK_PER_SECOND * 3);
//    goto AT_Config_Start;
//  }
  if(at_command(handle,buffer, "ATD*99#\r\n", "CONNECT") < 0) {
    rt_thread_delay(RT_TICK_PER_SECOND * 3);
    goto AT_Config_Start;
  }

  /*
  ** Step 16, �ͷ�ATռ�õ��ڴ�
  */
  rt_free(buffer);

  /*
  ** Step 17, һ��PPP���罨������ʼ����PPPЭ��ջ
  */
  device->try_connect_counter = 0;
  device->ppp = pppapi_pppos_create(&device->netif, pppos_output_cb, pppos_status_cb, device);
  pppapi_set_default(device->ppp);
  /* Auth configuration, this is pretty self-explanatory */
  ppp_set_auth(device->ppp, PPPAUTHTYPE_ANY, "edison", "edison");
  /* Ask the peer for up to 2 DNS server addresses. */
  ppp_set_usepeerdns(device->ppp, 1);

  /*
  * Initiate PPP negotiation, without waiting (holdoff=0), can only be called
  * if PPP session is in the dead state (i.e. disconnected).
  */
  holdoff = 0;
  pppapi_connect(device->ppp, holdoff);
  
  /*
  ** Step 18, ���ô�������������Ҫ�ڳ�ʱ����
  */
  len = 0;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &len);
  for(;;)
  {
    len = rt_device_read(handle, 0, device->buffer, 1024);
    if(len > 0) {
       pppos_input_tcpip(device->ppp, device->buffer, len);
    }
  }
}

/*********************************************************************************************************
** Function name:       lwip_pppos_init
** Descriptions:        PPPOS�豸��ʼ��
** input parameters:    ttyName: PPPOSʹ�õĴ���
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
int lwip_pppos_init(const char *ttyName)
{
  rt_thread_t tid;
  rt_device_t handle;
  pppos_device *device;
  struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
  
  /*
  ** Step 1, �ȷ���ppp�豸�Ľṹ
  */
  device = rt_malloc(sizeof(pppos_device));
  if(device == RT_NULL) {
    return -1;
  }
  
  /*
  ** Step 2, ���Ҳ��򿪴���
  */
  handle = rt_device_find(ttyName);
  if(handle == RT_NULL) {
    rt_kprintf("Can't find %s\r\n", ttyName);
    return -1;
  }
  rt_device_open(handle,RT_DEVICE_OFLAG_RDWR);
  
  /*
  ** Step 3, ���ô��ڲ�����
  */
  rt_device_control(handle, RT_SERIAL_CONFIG_SET, &cfg);
  
  /*
  ** Step 4, ��ʼ��ppp�ṹ�����������߳�
  */
  device->serial = handle;
  tid = rt_thread_create("ppp", pppos_rx_thread, device, RT_LWIP_PPPOS_STACKSIZE, RT_LWIP_PPPOS_PRIORITY, 20);
  
  if (tid != RT_NULL) {
    rt_thread_startup(tid);
  }
  return 1;
}

/*********************************************************************************************************
** Function name:       gprs_rssi_get
** Descriptions:        ��ȡGPRSģ����ź�ǿ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
rt_uint16_t gprs_rssi_get(void)
{
  return __gRSSI;
}

/*********************************************************************************************************
** Function name:       gprs_rssi_get
** Descriptions:        ��ȡGPRSģ����ź�ǿ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ccid����
*********************************************************************************************************/
int gprs_ccid_get(char *ccid)
{
  int len;
  
  len = strlen(__gCCID) + 1;
  memset(ccid, 0, len);
  strcpy(ccid, __gCCID);
  return len;
}


#endif   // end of RT_USE_PPPOS
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
