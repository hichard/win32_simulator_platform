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

#include "lwip_pppos_mux.h"

#include "board_serial.h"
#include "board_led.h"
#include "gsmmux/gsmmux_0710.h"

#ifdef RT_USE_PPPOS

/*********************************************************************************************************
**  ���Ŵ�������
*********************************************************************************************************/
#ifndef AT_TRY_NUM
#define AT_TRY_NUM               10
#endif

#ifndef PPP_TRY_NUM
#define PPP_TRY_NUM              3
#endif

/*********************************************************************************************************
** ��������궨��
*********************************************************************************************************/
#ifndef PPPOS_DEBUG
#define PPPOS_DEBUG   1
#endif

#if (PPPOS_DEBUG > 0)
#define pppos_trace(fmt, ...)           rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define pppos_trace(fmt, ...)
#endif

/*********************************************************************************************************
**  ���������ṹ
*********************************************************************************************************/
typedef struct {
  ppp_pcb *ppp;
  rt_device_t serial;
  char *apn_name;
  char *apn_password;
  rt_uint8_t  ppp_status;      //0:��ʼ��ģʽ��1��ATָ������ģʽ  2��PPP��Э��ģʽ
  rt_uint16_t try_connect_counter;
  struct netif netif;
  rt_uint8_t buffer[1500];
  int(*modem_at_dial)(rt_device_t handle,uint8_t *buffer);
}pppos_device;

/*********************************************************************************************************
**  ����ȫ�ֱ���
*********************************************************************************************************/

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
      pppos_trace("status_cb: Connected\n");
#if PPP_IPV4_SUPPORT
      pppos_trace("   our_ipaddr  = %s\n", ipaddr_ntoa(&pppif->ip_addr));
      pppos_trace("   his_ipaddr  = %s\n", ipaddr_ntoa(&pppif->gw));
      pppos_trace("   netmask     = %s\n", ipaddr_ntoa(&pppif->netmask));
#if LWIP_DNS
      ns = dns_getserver(0);
      ns = ns;
      pppos_trace("   dns1        = %s\n", ipaddr_ntoa(ns));
      ns = dns_getserver(1);
      pppos_trace("   dns2        = %s\n", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
      pppos_trace("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
      break;
    }
    case PPPERR_PARAM: {
      pppos_trace("status_cb: Invalid parameter\n");
      break;
    }
    case PPPERR_OPEN: {
      pppos_trace("status_cb: Unable to open PPP session\n");
      break;
    }
    case PPPERR_DEVICE: {
      pppos_trace("status_cb: Invalid I/O device for PPP\n");
      break;
    }
    case PPPERR_ALLOC: {
      pppos_trace("status_cb: Unable to allocate resources\n");
      break;
    }
    case PPPERR_USER: {
      pppos_trace("status_cb: User interrupt\n");
      break;
    }
    case PPPERR_CONNECT: {
      pppos_trace("status_cb: Connection lost\n");
      break;
    }
    case PPPERR_AUTHFAIL: {
      pppos_trace("status_cb: Failed authentication challenge\n");
      break;
    }
    case PPPERR_PROTOCOL: {
      pppos_trace("status_cb: Failed to meet protocol\n");
      break;
    }
    case PPPERR_PEERDEAD: {
      pppos_trace("status_cb: Connection timeout\n");
      break;
    }
    case PPPERR_IDLETIMEOUT: {
      pppos_trace("status_cb: Idle Timeout\n");
      break;
    }
    case PPPERR_CONNECTTIME: {
      pppos_trace("status_cb: Max connect time reached\n");
      break;
    }
    case PPPERR_LOOPBACK: {
      pppos_trace("status_cb: Loopback detected\n");
      break;
    }
    default: {
      pppos_trace("status_cb: Unknown error code %d\n", err_code);
      break;
    }
  }

/*
 * This should be in the switch case, this is put outside of the switch
 * case for example readability.
 */

  if (err_code == PPPERR_NONE) {
    device->try_connect_counter = 0;
#ifdef ppp_net_on
    ppp_net_on();
#endif
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
#ifdef ppp_net_off
    ppp_net_off();
#endif
  ppp_connect(pcb, 20);
  device->try_connect_counter++;
  if(device->try_connect_counter > PPP_TRY_NUM) {
    // ��λmodem
    ppp_close(pcb,0);
    gsmmux_reset();
    device->ppp_status = 0;
    
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
  u16_t holdoff = 0;
  rt_uint32_t len;
  rt_uint32_t at_try_count;
  int status;
  pppos_device *device = parg;
  rt_device_t handle = device->serial;

  /*
  ** Step 1, һ��PPP���罨������ʼ����PPPЭ��ջ
  */
  device->ppp_status = 0;
  device->try_connect_counter = 0;
  device->ppp = pppapi_pppos_create(&device->netif, pppos_output_cb, pppos_status_cb, device);
  pppapi_set_default(device->ppp);
  /* Auth configuration, this is pretty self-explanatory */
  ppp_set_auth(device->ppp, PPPAUTHTYPE_ANY, device->apn_name, device->apn_password);
  /* Ask the peer for up to 2 DNS server addresses. */
  ppp_set_usepeerdns(device->ppp, 1);
  
  for(;;)
  {
    /*
    ** Step 2, �ȵȴ�gsmmux����
    */
    while(gsmmux_status() == 0) {
      rt_thread_delay(RT_TICK_PER_SECOND);
    }
    
    /*
    ** Step 3, �ȴ�ppp���� 
    */
    if(device->ppp_status < 1) {
      at_try_count = 0;      
      do {
        status = device->modem_at_dial(handle, device->buffer);
        at_try_count++;
        if(at_try_count > AT_TRY_NUM) {
          gsmmux_reset();
          device->ppp_status = 0;
          break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND);
      } while(status < 1);
      if(status > 0) {
        device->ppp_status = 1;
      }
    }
    
    /*
    ** Step 3, ppp����
    */
    if(device->ppp_status == 1) {
      holdoff = 0;
      pppapi_connect(device->ppp, holdoff);
      device->ppp_status = 2;
    }
  
    /*
    ** Step 4, ��Э�̣��ȴ�ppp�������ӳɹ�
    */
    len = 2000;
    rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &len);
    len = 0;
    rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &len);
    while(device->ppp_status > 1) {
      len = rt_device_read(handle, 0, device->buffer, 1500);
      if(len > 0) {
        pppos_input_tcpip(device->ppp, device->buffer, len);
      }
    }
  }
}

/*********************************************************************************************************
** Function name:       lwip_pppos_mux_init
** Descriptions:        PPPOS�豸��ʼ��
** input parameters:    ttyName: PPPOSʹ�õĴ���
**                      modem_at_dial�� �������ӵ�atָ��ִ������
**                      apn_name:       APN�û���
**                      apn_password:   APN����
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
int lwip_pppos_mux_init(const char *ttyName, int(*modem_at_dial)(rt_device_t handle,uint8_t *buffer), 
                        char * apn_name, char *apn_password)
{
  rt_thread_t tid;
  rt_device_t handle;
  pppos_device *device;
  
  /*
  ** Step 1, �ȷ���ppp�豸�Ľṹ
  */
  device = rt_malloc(sizeof(pppos_device));
  if(device == RT_NULL) {
    return -1;
  }
  memset(device, 0, sizeof(pppos_device));
  
  /*
  ** Step 2, ���Ҳ��򿪴���
  */
  handle = rt_device_find(ttyName);
  if(handle == RT_NULL) {
    pppos_trace("Can't find %s\r\n", ttyName);
    return -1;
  }
  rt_device_open(handle,RT_DEVICE_OFLAG_RDWR);
  
  /*
  ** Step 3, ��ʼ��ppp�ṹ�����������߳�
  */
  device->serial = handle;
  device->modem_at_dial = modem_at_dial;
  device->apn_name = apn_name;
  device->apn_password = apn_password;
  tid = rt_thread_create("ppp", pppos_rx_thread, device, RT_LWIP_PPPOS_STACKSIZE, RT_LWIP_PPPOS_PRIORITY, 20);
  
  if (tid != RT_NULL) {
    rt_thread_startup(tid);
  }
  return 1;
}

#endif   // end of RT_USE_PPPOS
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
