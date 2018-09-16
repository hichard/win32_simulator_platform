/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           gsmmux_0710.c
** Last modified Date:  2017-11-11
** Last Version:        v1.00
** Description:         gsm0710��������ʵ��
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2017-11-11
** Version:             v1.00
** Descriptions:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:                
** Description:         
*********************************************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "board_serial.h"

#include "gsmmux_opts.h"
#include "gsmmux_buffer.h"
#include "gsmmux_0710.h"

/*********************************************************************************************************
** ��������궨��
*********************************************************************************************************/
#if (GSMMUX_0710_DEBUG > 0)
#define gsmmux_0710_trace(fmt, ...)           rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define gsmmux_0710_trace(fmt, ...)
#endif

/*********************************************************************************************************
**  gsm0710֡��ʽ����
*********************************************************************************************************/
typedef struct gsm0710_frame
{
    uint8_t  channel;
    uint8_t  control;
    uint16_t data_length;
    uint8_t  *data;
} gsm0710_frame_t;

/*********************************************************************************************************
**  gsmmuxʵ�������ṹ����
*********************************************************************************************************/
typedef struct gsmmux_0710 {
  rt_device_t handle;                           // ���ھ��
  
  struct rt_mutex send_lock;                   // gsm0710���ͱ�����
  uint8_t send_buffer[GSM0710_BUFFER_NUM];      // ���ͻ�����
  uint8_t recv_buffer[GSM0710_BUFFER_NUM];      // ���ջ�����

  void(*power_ctrl)(void);                    // ��������
  int(*enter_mux)(rt_device_t handle,uint8_t *buffer);// ����muxģʽ
}gsmmux_0710_t;

/*********************************************************************************************************
**  gsmmux���⴮����������
*********************************************************************************************************/
typedef struct gsmmux_serial {
  struct gsmmux_0710 *gsm0710;
  
  uint8_t  channel;
  uint8_t  opened;                              //  �߼�ͨ���Ƿ��                  
  uint8_t  v24_signals;                         //  V.24������Ϣ
  uint32_t RecvTimeoutSave;                        //  ���ճ�ʱ���������棬�������ñ���
  uint32_t RecvIntervalTimeSave;                //  �������ʱ����
  struct gsmmux_buffer serial_buffer;
}gsmmux_serial_t;

/*********************************************************************************************************
**  ȫ�ֱ�������
*********************************************************************************************************/
// gsm0710���ջ�����
static uint8_t __gserial_read_buffer[GSM0710_BUFFER_NUM];
// ����gsm0710ʵ�������ṹ
struct gsmmux_0710 __ggsmmux_0710_use;

// �߼�ͨ�������ṹ����������ͨ��
struct gsmmux_serial __ggsmmux_serial_channel[GSM0710_PORT_NUM + 1];
// �߼�ͨ��1������������ppp���ţ���Ҫ�㹻��
static uint8_t gsm_channel_1_buffer[4096];
// �����߼�ͨ��������������ATָ�����Ҫ�ܴ�
static uint8_t gsm_channle_else_buffer[GSM0710_PORT_NUM - 1][1024];

// gsmmux״̬   0:��ʼ״̬��  1��׼������
uint8_t __ggsmmux_status = 0;


/*********************************************************************************************************
** Function name:       gsmmux_write_frame
** Descriptions:        gsmmuxͨ����ʼ��
** Input parameters:    ttyName��   ʹ�õĴ���
**                      baud:       ���ڲ�����
**                      power_ctrl��modem��Դ����
**                      enter_mux�� ����muxģʽ����
** Output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int gsmmux_write_frame(struct gsmmux_0710 *pgsm, uint8_t channel, uint8_t control, 
                                 const uint8_t *buffer, uint16_t length)
{
#if (GSM0710_BASIC > 0)
    uint8_t crc8;
    uint16_t pos;
    uint8_t *psend_buf = pgsm->send_buffer;
    
    
    rt_mutex_take(&pgsm->send_lock, RT_WAITING_FOREVER);
    pos = 0;
    psend_buf[pos++] = GSM0710_BASIC_FLAG;
    psend_buf[pos++] = (((channel & 0x3F) << 2) | (GSM0710_EA | GSM0710_CR));
    psend_buf[pos++] = control;
    // ȥ��ͷ����β���ĳ��ȣ����ݲ��ܳ������ͻ�������С
    if(length > (GSM0710_BUFFER_NUM - 8)) {
      length =  (GSM0710_BUFFER_NUM - 8);
    }
    if (length > 0x7F)
    {
        psend_buf[pos++] = (length & 0x7F) << 1;
        psend_buf[pos++] = (length & 0x7F80) >> 7;
    }
    else
    {
        psend_buf[pos++] = (length << 1) | 0x01;
    }
    
    crc8 = gsmmux_make_fcs(&psend_buf[1], pos - 1, 0x00);
    // UI֡��ҪУ�����ݲ��֣���������Ҫ
    if(GSM0710_FRAME_IS(control, GSM0710_UI)) {
      if(length > 0) {
          crc8 = gsmmux_make_fcs(buffer, length, crc8);
      }
    }
    
    if(length > 0) {
      memcpy(&psend_buf[pos], buffer, length);
      pos += length;
    }
    
    psend_buf[pos++] = crc8;
    psend_buf[pos++] = GSM0710_BASIC_FLAG;
    
    rt_device_write(pgsm->handle,0,psend_buf,pos);
    
    rt_mutex_release(&pgsm->send_lock);
    
    return length;
#endif
}


/*********************************************************************************************************
** Function name:       gsmmux_channel_init
** Descriptions:        gsmmuxͨ����ʼ��
** Input parameters:    pgsm: gsm0710�����ṹ
** Output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void gsmmux_channel_init(struct gsmmux_0710 *pgsm)
{
  int i;
  uint32_t nretry = 0;
  rt_device_t handle = pgsm->handle;
  
  __ggsmmux_status = 0;
  
  // ���ͨ��״̬
  for(i=0; i<(GSM0710_PORT_NUM + 1); i++) 
  {
    __ggsmmux_serial_channel[i].opened = 0;
    __ggsmmux_serial_channel[i].v24_signals = 0;
  }
  
  pgsm->power_ctrl();
  rt_thread_delay(RT_TICK_PER_SECOND * 3);
  rt_thread_delay(RT_TICK_PER_SECOND * 10);
  while(!pgsm->enter_mux(handle, __gserial_read_buffer)) 
  {
    nretry += 6;
    // ����ָ��ʱ�䶼�޷�������ӦATָ�����MUXģʽ
    if(nretry > (GSM0710_MODEM_WAIT_TIME)) {
      return;
    }
    rt_thread_delay(RT_TICK_PER_SECOND * 1);
  }
  
  gsmmux_0710_trace("modem init finish!\r\n");
  
  for (i = 0; i < (GSM0710_PORT_NUM + 1); i++)
  {
    gsmmux_0710_trace("Opening channel %d.\n", i);
    gsmmux_write_frame(pgsm, i, GSM0710_SABM | GSM0710_PF, RT_NULL, 0);
  }
  
  gsmmux_0710_trace("gsmmux init finish!\r\n");
  
  
  __ggsmmux_status = 1;
}

/*********************************************************************************************************
** Function name:       gsm0710_prase_frame
** Descriptions:        gsm0710֡��ʽ����
** input parameters:    frame:  ֡��ʽ������Žṹ
**                      buffer: �������ݻ�����
**                      length���������ݳ���
** output parameters:   NONE
** Returned value:      -1: ����δ������ϣ�����Ҫ��������
**                      0�����ݽ�������crc����
**                      1��������ȷ����
*********************************************************************************************************/
static int gsm0710_prase_frame(struct gsm0710_frame *frame, uint8_t *buffer,uint8_t length)
{
  uint8_t prefix_length = 3;
  uint32_t u32Len;
  
  // address, control, length, fcs
  if (length < 4)
  {
    return -1;
  }
  
  frame->channel = buffer[0] >> 2;
  if(frame->channel > GSM0710_PORT_NUM) {
     gsmmux_0710_trace("gsmmux parse_frame no support channel!\r\n");
     return 0;
  }
  frame->control = buffer[1];
  frame->data_length = buffer[2] >> 1;
  if (!(buffer[2] & GSM0710_EA))
  {
    prefix_length = 4;
    frame->data_length |= (uint16_t)buffer[3] << 7;
  }
  if (length < prefix_length + frame->data_length + 1)
  {
    return -1;
  }
  
  u32Len = GSM0710_FRAME_IS(frame->control, GSM0710_UI) ? (prefix_length + frame->data_length) : prefix_length;
  if (gsmmux_make_fcs(buffer, u32Len, 0x00) != buffer[prefix_length + frame->data_length])
  {
    gsmmux_0710_trace("gsmmux parse_frame crc error!\r\n");
    return 0;
  }
  
  frame->data = &buffer[prefix_length];
  return 1;
}

/*********************************************************************************************************
** Function name:       gsm0710_command_handle
** Descriptions:        gsm0710����֡����
** input parameters:    pgsm:   gsm0710�����ṹ
**                      frame:  �������GSM0710֡
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void gsm0710_command_handle(struct gsmmux_0710 *pgsm, gsm0710_frame_t *frame)
{
  uint8_t type;
  uint16_t length;
  uint16_t offset;
  
  uint16_t type_length = 0;
  uint8_t supported = 1;
  
  if (frame->data_length > 0)
  {
    type = frame->data[0];  // only a byte long types are handled now
    // skip extra bytes
    for (offset = 0; ((frame->data_length > offset) && ((frame->data[offset] & GSM0710_EA) == 0)); offset++);
    offset++;
    type_length = offset;
    
    // extract frame length
    length = 0;
    while (frame->data_length > offset)
    {
      length = (length << 7) + (frame->data[offset] >> 1);
      if (frame->data[offset] & GSM0710_EA) break;
      offset++;
    }
    offset++;
    
    if (type & GSM0710_CR) // command not ack
    {
      switch (type & ~GSM0710_CR)
      {
      case GSM0710_CMD_PSC:
        gsmmux_0710_trace("The mobile station requested power saving.\n");
        break;
      case GSM0710_CMD_CLD:
        gsmmux_0710_trace("The mobile station requested mux-mode termination.\n");
        break;
      case GSM0710_CMD_TEST:
        if (offset + length <= frame->data_length)
        {
          gsmmux_0710_trace("Test command: ");
          gsmmux_0710_trace("data = %s  / length = %d\n", &frame->data[offset], length);
        }
        else
        {
          gsmmux_0710_trace("ERROR: Test command, but no info. offset: %d, len: %d, data-len: %d\n", offset, length, frame->data_length);
        }
        break;
      case GSM0710_CMD_MSC:
        if (offset + length <= frame->data_length)
        {
          uint8_t channel = frame->data[offset] >> 2;
          uint8_t signals = frame->data[offset + 1];
          
          __ggsmmux_serial_channel[channel].v24_signals = signals;
          gsmmux_0710_trace("Modem status command on channel %d.\n", channel);
          if (signals & GSM0710_V_24_FC)
          {
            gsmmux_0710_trace("No frames allowed.\n");
          }
          else
          {
            gsmmux_0710_trace("Frames allowed.\n");
          }
          if (signals & GSM0710_V_24_RTC)
          {
            gsmmux_0710_trace("RTC\n");
          }
          if (signals & GSM0710_V_24_IC)
          {
            gsmmux_0710_trace("Ring\n");
          }
          if (signals & GSM0710_V_24_DV)
          {
            gsmmux_0710_trace("DV\n");
          }
        }
        else
        {
          gsmmux_0710_trace("ERROR: Modem status command, but no info. offset: %d, len: %d, data-len: %d\n", offset, length, frame->data_length);
        }
        break;
      case GSM0710_CMD_FCOFF:
        gsmmux_0710_trace("The mobile station requested flow control off.\n");
        break;
      case GSM0710_CMD_FCON:
        gsmmux_0710_trace("The mobile station requested flow control on.\n");
        break;
      default:
        gsmmux_0710_trace("Unknown command (0x%02X) from the control channel.\n", type);
        supported = 0;
        break;
      }
      
      if (supported)
      {
        // acknowledge the command
        frame->data[0] &= ~GSM0710_CR;
        gsmmux_write_frame(pgsm, 0, GSM0710_UIH, frame->data, frame->data_length);
      }
      else
      {
        // supposes that type length is not more than 8
        if (type_length <= 8)
        {
          uint8_t i;
          uint8_t response[10];
          
          response[0] = GSM0710_CMD_NSC;
          response[1] = GSM0710_EA | (type_length << 1);
          for (i = 0; i < type_length; i++)
          {
            response[2 + i] = frame->data[i];
          }
          gsmmux_write_frame(pgsm, 0, GSM0710_UIH, response, 2 + type_length);
        }
      }
    }
    else
    {
      // received ack for a command
      if (GSM0710_COMMAND_IS(type, GSM0710_CMD_NSC))
      {
        gsmmux_0710_trace("The mobile station didn't support the command sent.\n");
      }
      else
      {
        gsmmux_0710_trace("Command acknowledged by the mobile station.\n");
      }
    }
  }
}

/*********************************************************************************************************
** Function name:       gsmmux_handler
** Descriptions:        gsmmux����֡����
** input parameters:    parg: �̴߳������
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void gsmmux_handler(uint8_t channel, const void *pbuf, uint32_t length)
{
  if ((channel < 1) || (channel > GSM0710_PORT_NUM)) {
    return;
  }
  gsmmux_buffer_write(&__ggsmmux_serial_channel[channel].serial_buffer, pbuf, length);
}

/*********************************************************************************************************
** Function name:       gsmmux_recv_thread
** Descriptions:        ��ʼ�����⴮��
** input parameters:    parg: �̴߳������
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void gsmmux_recv_thread(void *parg)
{
#if (GSM0710_BASIC > 0)    // GSM0710��������֡��ʽ
  uint16_t i;
  uint32_t u32RecvCount = 0;
  uint32_t u32Len;
  struct gsmmux_0710 *pgsm = (struct gsmmux_0710 *)parg;
  uint8_t *this_buffer = pgsm->recv_buffer;
  rt_device_t handle = pgsm->handle;
  gsm0710_frame_t frame;
  
  for(;;)
  {
    /*
    ** ���ó�ʼ��ʱ���ڵ�ִ�в���
    */
    u32Len = 5000;
    rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
    u32Len = 500;
    rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &u32Len);
    
    // ���gsmmux״̬Ϊ0����ʼ��modem
    while(__ggsmmux_status == 0) 
    {
      gsmmux_channel_init(pgsm);
    }
    
    /*
    ** ��ʼ����ɣ�����Ӧ�õĳ�ʼ������
    */
    u32Len = 2000;
    rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
    u32Len = 0;
    rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &u32Len);
    
    // ���gsmmux״̬Ϊ1�� �ַ�mux����
    u32RecvCount = 0;
    while(__ggsmmux_status == 1)
    {
      u32Len = rt_device_read(handle, 0, __gserial_read_buffer, GSM0710_BUFFER_NUM);
      if(u32Len <= 0) 
      {
        continue;
      }
      
      for (i = 0; i < u32Len; i++)
      {
        if (__gserial_read_buffer[i] != GSM0710_BASIC_FLAG)
        {
          if (u32RecvCount >= GSM0710_BUFFER_NUM)
          {
            gsmmux_0710_trace("gsmmux recv buffer overflow!\r\n");
            u32RecvCount = 0;
          }
          this_buffer[u32RecvCount++] = __gserial_read_buffer[i];
        }
        else if (u32RecvCount > 0)
        {
          int res = gsm0710_prase_frame(&frame, this_buffer, u32RecvCount);
          if (res < 0)
          {
            this_buffer[u32RecvCount++] = __gserial_read_buffer[i];
          }
          else
          {
            if (res > 0)
            {
              if (GSM0710_FRAME_IS(frame.control, GSM0710_UI) || GSM0710_FRAME_IS(frame.control, GSM0710_UIH))
              {
                if (frame.channel == 0) // control channel
                {
                  gsm0710_command_handle(pgsm, &frame);
                }
                else // logical channel
                {
                  gsmmux_handler(frame.channel, frame.data, frame.data_length);
                }
              }
              else
              {
                switch (frame.control & ~GSM0710_PF)
                {
                case GSM0710_UA: // Unnumbered Acknowledgement
                  if(__ggsmmux_serial_channel[frame.channel].opened) 
                  {
                    __ggsmmux_serial_channel[frame.channel].opened = 0;
                    gsmmux_0710_trace("gsmmux channel %d closed\r\n", frame.channel);
                  } else {
                    __ggsmmux_serial_channel[frame.channel].opened = 1;
                    gsmmux_0710_trace("gsmmux channel %d opened\r\n", frame.channel);
                  }
                  break;
                case GSM0710_DM: // Disconnected Mode
                  if(__ggsmmux_serial_channel[frame.channel].opened) 
                  {
                    __ggsmmux_serial_channel[frame.channel].opened  = 0;
                    gsmmux_0710_trace("DM received, so the channel %d was already closed.\n", frame.channel);
                  }
                  else
                  {
                    gsmmux_0710_trace("channel %d couldn't be opened.\n", frame.channel);
                  }
                  break;
                case GSM0710_DISC: // Disconnect
                  if(__ggsmmux_serial_channel[frame.channel].opened)
                  {
                    __ggsmmux_serial_channel[frame.channel].opened = 0;
                    gsmmux_write_frame(pgsm, frame.channel, GSM0710_UA | GSM0710_PF, RT_NULL, 0);
                    gsmmux_0710_trace("channel %d closed.\n", frame.channel);
                  }
                  else
                  {
                    gsmmux_write_frame(pgsm, frame.channel, GSM0710_DM | GSM0710_PF, NULL, 0);
                    gsmmux_0710_trace("Received DISC even though channel %d was already closed.\n", frame.channel);
                  }
                  break;
                case GSM0710_SABM: // Set Asynchronous Balanced Mode
                  if (!__ggsmmux_serial_channel[frame.channel].opened)
                  {
                    __ggsmmux_serial_channel[frame.channel].opened = 1;
                    gsmmux_0710_trace("channel %d opened.\n", frame.channel);
                  }
                  else
                  {
                    gsmmux_0710_trace("Received SABM even though channel %d was already opened.\n", frame.channel);
                  }
                  gsmmux_write_frame(pgsm, frame.channel, GSM0710_UA | GSM0710_PF, NULL, 0);
                  break;
                }
              }
            }
            u32RecvCount = 0;
          }
        }
      }
    }
  }
#endif
}


/*********************************************************************************************************
** Function name:       gsmmux_serial_init
** Descriptions:        ��ʼ�����⴮��
** input parameters:    dev: �豸�����ṹ
** output parameters:   NONE
** Returned value:      RT_EOK: �����ɹ�;  ����, ����ʧ��
*********************************************************************************************************/
static rt_err_t gsmmux_serial_init (rt_device_t dev)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_open
** Descriptions:        �򿪴���
** input parameters:    dev: �豸�����ṹ
** output parameters:   NONE
** Returned value:      RT_EOK: �����ɹ�;  ����, ����ʧ��
*********************************************************************************************************/
static rt_err_t gsmmux_serial_open(rt_device_t dev, rt_uint16_t oflag)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_close
** Descriptions:        �رմ���
** input parameters:    dev: �豸�����ṹ
** output parameters:   NONE
** Returned value:      RT_EOK: �����ɹ�;  ����, ����ʧ��
*********************************************************************************************************/
static rt_err_t gsmmux_serial_close(rt_device_t dev)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_read
** Descriptions:        �Ӵ��ڶ�ȡָ���ֽڵ�����
** input parameters:    dev: ��������������
**                      pos:    ����ƫ�Ƶ�ַ�����ڴ��ڣ���ֵ��Ч
**                      buffer: ��ȡ���ݴ�ŵ�ַ
**                      size:       ��ȡ���ݳ���
** output parameters:   ʵ�ʽ��յ������ݳ���
*********************************************************************************************************/
static rt_size_t gsmmux_serial_read (rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
  uint32_t len;
  struct gsmmux_serial *serial_handle = dev->user_data;
  
  if ((serial_handle->channel < 1) || (serial_handle->channel > GSM0710_PORT_NUM))
  {
    return 0;
  }
  
  len = gsmmux_buffer_read(&serial_handle->serial_buffer, buffer, size, 
                           serial_handle->RecvTimeoutSave,serial_handle->RecvIntervalTimeSave);
  
  return len;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_write
** Descriptions:        �Ӵ��ڷ���ָ���ֽڵ����ݣ����߲���DMA���ͣ����߲����жϷ���
** input parameters:    dev: ��������������
**                      pos:    ����ƫ�Ƶ�ַ�����ڴ��ڣ���ֵ��Ч
**                      buffer: �������ݴ�ŵ�ַ
**                      size:       �������ݳ���
** output parameters:   ʵ�ʷ��͵����ݳ���
*********************************************************************************************************/
static rt_size_t gsmmux_serial_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
  struct gsmmux_serial *serial_handle = dev->user_data;
  
  if ((serial_handle->channel < 1) || (serial_handle->channel > GSM0710_PORT_NUM))
  {
    return 0;
  }
  
  if ((!serial_handle->opened) || (serial_handle->v24_signals & GSM0710_V_24_FC))
  {
    return 0;
  }
  
  gsmmux_write_frame(serial_handle->gsm0710, serial_handle->channel, GSM0710_UIH, buffer, size);
  
  return size;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_control
** Descriptions:        ���ڿ��ƣ����������ò����ʣ����ݸ�ʽ��
** input parameters:    dev:     ��������������
**                      cmd:    ��������
**                      args:     ���ò���
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_err_t gsmmux_serial_control (rt_device_t dev, int cmd, void *args)
{
  rt_err_t err_code = RT_EOK;
  rt_base_t level;
  struct gsmmux_serial *serial_handle = dev->user_data;
  
  RT_ASSERT(dev != RT_NULL);
  
  switch (cmd)
  {
  case RT_SERIAL_TIMEOUT_SET:
    {
      rt_uint32_t count = *((rt_uint32_t *)args);
      rt_uint32_t temp;
      
      if(args == RT_NULL) {
        err_code = RT_ERROR;
        break;
      }
      
      // ��msת��Ϊʱ��tick
      temp = 1000/RT_TICK_PER_SECOND;
      count = (count % temp)?((count / temp) + 1):(count / temp);
      /* disable interrupt */
      level = rt_hw_interrupt_disable();
      serial_handle->RecvTimeoutSave = count;
      /* enable interrupt */
      rt_hw_interrupt_enable(level);
      break;
    }
  case RT_SERIAL_TIME_INTERVAL_SET:
   {
      rt_uint32_t count = *((rt_uint32_t *)args);
      rt_uint32_t temp;
      
      if(args == RT_NULL) {
        err_code = RT_ERROR;
        break;
      }
      
      // ��msת��Ϊʱ��tick
      temp = 1000/RT_TICK_PER_SECOND;
      count = (count % temp)?((count / temp) + 1):(count / temp);
      /* disable interrupt */
      level = rt_hw_interrupt_disable();
      serial_handle->RecvIntervalTimeSave = count;
      /* enable interrupt */
      rt_hw_interrupt_enable(level);
      break;
    }
  default:
    err_code = RT_ERROR;
    break;
  }
  
  return err_code;
}

/*********************************************************************************************************
** Function name:       gsmmux_reset
** Descriptions:        gsmmux��λ�������³�ʼ�������½���gsmmux�߼�ͨ��
** Input parameters:    NONE
** Output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void gsmmux_reset(void)
{
  __ggsmmux_status = 0;
}

/*********************************************************************************************************
** Function name:       gsmmux_status
** Descriptions:        ��ȡgsmmux��״̬
** Input parameters:    NONE
** Output parameters:   NONE
** Returned value:      0����ʼ���У� 1��ͨ�������ɹ�
*********************************************************************************************************/
uint8_t gsmmux_status(void)
{
   return __ggsmmux_status;
}

/*********************************************************************************************************
** Function name:       gsmmux_init
** Descriptions:        gsmmux��ʼ��
** Input parameters:    name:       ʹ�õĴ���
**                      baud:       ���ڲ�����
**                      power_ctrl��modem��Դ����
**                      enter_mux������muxģʽ����
** Output parameters:   None ��
** Returned value:      -1: ָ���Ĵ��ڲ�����
**                      -2: �ڴ治��
*********************************************************************************************************/
int gsmmux_init(const char *name,uint32_t baud, 
                void(*power_ctrl)(void), int(*enter_mux)(rt_device_t handle, uint8_t *buffer))
{
  char mux_name[RT_NAME_MAX];
  int i;
  rt_device_t handle;
  rt_device_t device;
  rt_thread_t tid;
  struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
  
  /*
  ** Step 1, ��ʼ�������豸
  */
  if(__ggsmmux_0710_use.handle == RT_NULL) {
     handle = rt_device_find(name);
     if(handle == RT_NULL) {
       return -1;
     }
     rt_device_open(handle,RT_DEVICE_OFLAG_RDWR);
     cfg.baud_rate = baud;
     rt_device_control(handle, RT_SERIAL_CONFIG_SET, &cfg);
     __ggsmmux_0710_use.handle = handle;
     
     rt_mutex_init(&__ggsmmux_0710_use.send_lock, "0710lock", RT_IPC_FLAG_FIFO);
     __ggsmmux_0710_use.power_ctrl =  power_ctrl;
     __ggsmmux_0710_use.enter_mux = enter_mux;

     // ��ʼ������ͨ��
     __ggsmmux_serial_channel[0].gsm0710 = &__ggsmmux_0710_use;
     __ggsmmux_serial_channel[0].channel = 0;
     __ggsmmux_serial_channel[0].opened = 0;
     __ggsmmux_serial_channel[0].v24_signals = 0;
     __ggsmmux_serial_channel[0].RecvTimeoutSave = 0;
     __ggsmmux_serial_channel[0].RecvIntervalTimeSave = 0;
      gsmmux_buffer_init(&(__ggsmmux_serial_channel[0].serial_buffer), RT_NULL, 0);
     
     // ��ʼ��ppp�߼�ͨ���ṹ
     __ggsmmux_serial_channel[1].gsm0710 = &__ggsmmux_0710_use;
     __ggsmmux_serial_channel[1].channel = 1;
     __ggsmmux_serial_channel[1].opened = 0;
     __ggsmmux_serial_channel[1].v24_signals = 0;
     __ggsmmux_serial_channel[1].RecvTimeoutSave = 0;
     __ggsmmux_serial_channel[1].RecvIntervalTimeSave = 0;
     gsmmux_buffer_init(&(__ggsmmux_serial_channel[1].serial_buffer), gsm_channel_1_buffer, 4096);
     
     // ��ʼ�������߼�ͨ��������
     for(i=2; i<(GSM0710_PORT_NUM + 1); i++) 
     {
       __ggsmmux_serial_channel[i].gsm0710 = &__ggsmmux_0710_use;
       __ggsmmux_serial_channel[i].channel = i;
       __ggsmmux_serial_channel[i].opened = 0;
       __ggsmmux_serial_channel[i].v24_signals = 0;
       __ggsmmux_serial_channel[i].RecvTimeoutSave = 0;
       __ggsmmux_serial_channel[i].RecvIntervalTimeSave = 0;
       gsmmux_buffer_init(&(__ggsmmux_serial_channel[i].serial_buffer), gsm_channle_else_buffer[i-2], 1024);
     }
  }
  
  /*
  ** Step 2, ע�����ϵͳ�����豸
  */
  for(i=0; i<GSM0710_PORT_NUM; i++) 
  {
    sprintf(mux_name, "gsmmux%d", i+1);
    handle = rt_device_find(mux_name);
    // ��������ڣ���ע���豸
    if(handle == RT_NULL) {
       device = rt_malloc(sizeof(struct rt_device));
       if(device == RT_NULL) {
          gsmmux_0710_trace("No mem for register %s\r\n", mux_name);
          return -2;
       }
       device->type             = RT_Device_Class_Char;
       device->rx_indicate      = RT_NULL;
       device->tx_complete      = RT_NULL;
       device->init             = gsmmux_serial_init;
       device->open             = gsmmux_serial_open;
       device->close            = gsmmux_serial_close;
       device->read             = gsmmux_serial_read;
       device->write            = gsmmux_serial_write;
       device->control          = gsmmux_serial_control;
       device->user_data        = (void *)&__ggsmmux_serial_channel[i+1];
      
       /* register a character device */
       rt_device_register(device, mux_name, RT_DEVICE_FLAG_RDWR);
    }
  }
  
  /*
  ** Step 3, ���������߳�
  */
  tid = rt_thread_find("muxrecv");
  // ��������ڣ��򴴽��߳�
  if(tid == RT_NULL) {
    tid = rt_thread_create("muxrecv", gsmmux_recv_thread, &__ggsmmux_0710_use, 1024, 3, 20);
    
    if (tid != RT_NULL) {
      rt_thread_startup(tid);
    }
  }
  
  return 0;
}

/*********************************************************************************************************
END FILE
*********************************************************************************************************/
