/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           gsmmux_0710.c
** Last modified Date:  2017-11-11
** Last Version:        v1.00
** Description:         gsm0710基本函数实现
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
** 调试输出宏定义
*********************************************************************************************************/
#if (GSMMUX_0710_DEBUG > 0)
#define gsmmux_0710_trace(fmt, ...)           rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define gsmmux_0710_trace(fmt, ...)
#endif

/*********************************************************************************************************
**  gsm0710帧格式定义
*********************************************************************************************************/
typedef struct gsm0710_frame
{
    uint8_t  channel;
    uint8_t  control;
    uint16_t data_length;
    uint8_t  *data;
} gsm0710_frame_t;

/*********************************************************************************************************
**  gsmmux实现特征结构定义
*********************************************************************************************************/
typedef struct gsmmux_0710 {
  rt_device_t handle;                           // 串口句柄
  
  struct rt_mutex send_lock;                   // gsm0710发送保护锁
  uint8_t send_buffer[GSM0710_BUFFER_NUM];      // 发送缓冲区
  uint8_t recv_buffer[GSM0710_BUFFER_NUM];      // 接收缓冲区

  void(*power_ctrl)(void);                    // 开机控制
  int(*enter_mux)(rt_device_t handle,uint8_t *buffer);// 进入mux模式
}gsmmux_0710_t;

/*********************************************************************************************************
**  gsmmux虚拟串口特征定义
*********************************************************************************************************/
typedef struct gsmmux_serial {
  struct gsmmux_0710 *gsm0710;
  
  uint8_t  channel;
  uint8_t  opened;                              //  逻辑通道是否打开                  
  uint8_t  v24_signals;                         //  V.24控制信息
  uint32_t RecvTimeoutSave;                        //  接收超时计数器保存，用于配置保存
  uint32_t RecvIntervalTimeSave;                //  接收最大时间间隔
  struct gsmmux_buffer serial_buffer;
}gsmmux_serial_t;

/*********************************************************************************************************
**  全局变量定义
*********************************************************************************************************/
// gsm0710接收缓冲区
static uint8_t __gserial_read_buffer[GSM0710_BUFFER_NUM];
// 定义gsm0710实现特征结构
struct gsmmux_0710 __ggsmmux_0710_use;

// 逻辑通道特征结构，包括控制通道
struct gsmmux_serial __ggsmmux_serial_channel[GSM0710_PORT_NUM + 1];
// 逻辑通道1缓冲区，用于ppp拨号，需要足够大
static uint8_t gsm_channel_1_buffer[4096];
// 其它逻辑通道缓冲区，用于AT指令，不需要很大
static uint8_t gsm_channle_else_buffer[GSM0710_PORT_NUM - 1][1024];

// gsmmux状态   0:初始状态；  1：准备就绪
uint8_t __ggsmmux_status = 0;


/*********************************************************************************************************
** Function name:       gsmmux_write_frame
** Descriptions:        gsmmux通道初始化
** Input parameters:    ttyName：   使用的串口
**                      baud:       串口波特率
**                      power_ctrl：modem电源控制
**                      enter_mux： 进入mux模式控制
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
    // 去掉头部和尾部的长度，数据不能超过发送缓冲区大小
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
    // UI帧需要校验数据部分，其它不需要
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
** Descriptions:        gsmmux通道初始化
** Input parameters:    pgsm: gsm0710特征结构
** Output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void gsmmux_channel_init(struct gsmmux_0710 *pgsm)
{
  int i;
  uint32_t nretry = 0;
  rt_device_t handle = pgsm->handle;
  
  __ggsmmux_status = 0;
  
  // 清除通道状态
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
    // 超过指定时间都无法进入响应AT指令进入MUX模式
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
** Descriptions:        gsm0710帧格式解析
** input parameters:    frame:  帧格式解析存放结构
**                      buffer: 接收数据缓冲区
**                      length：接收数据长度
** output parameters:   NONE
** Returned value:      -1: 数据未接收完毕，还需要继续接收
**                      0：数据解析错误，crc错误
**                      1：数据正确解析
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
** Descriptions:        gsm0710命令帧处理
** input parameters:    pgsm:   gsm0710特征结构
**                      frame:  待处理的GSM0710帧
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
** Descriptions:        gsmmux数据帧处理
** input parameters:    parg: 线程传入参数
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
** Descriptions:        初始化虚拟串口
** input parameters:    parg: 线程传入参数
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void gsmmux_recv_thread(void *parg)
{
#if (GSM0710_BASIC > 0)    // GSM0710基本数据帧格式
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
    ** 设置初始化时串口的执行参数
    */
    u32Len = 5000;
    rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
    u32Len = 500;
    rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &u32Len);
    
    // 如果gsmmux状态为0，初始化modem
    while(__ggsmmux_status == 0) 
    {
      gsmmux_channel_init(pgsm);
    }
    
    /*
    ** 初始化完成，设置应用的初始化参数
    */
    u32Len = 2000;
    rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
    u32Len = 0;
    rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &u32Len);
    
    // 如果gsmmux状态为1， 分发mux数据
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
** Descriptions:        初始化虚拟串口
** input parameters:    dev: 设备驱动结构
** output parameters:   NONE
** Returned value:      RT_EOK: 操作成功;  其它, 操作失败
*********************************************************************************************************/
static rt_err_t gsmmux_serial_init (rt_device_t dev)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_open
** Descriptions:        打开串口
** input parameters:    dev: 设备驱动结构
** output parameters:   NONE
** Returned value:      RT_EOK: 操作成功;  其它, 操作失败
*********************************************************************************************************/
static rt_err_t gsmmux_serial_open(rt_device_t dev, rt_uint16_t oflag)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_close
** Descriptions:        关闭串口
** input parameters:    dev: 设备驱动结构
** output parameters:   NONE
** Returned value:      RT_EOK: 操作成功;  其它, 操作失败
*********************************************************************************************************/
static rt_err_t gsmmux_serial_close(rt_device_t dev)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       gsmmux_serial_read
** Descriptions:        从串口读取指定字节的数据
** input parameters:    dev: 外设特性描述符
**                      pos:    数据偏移地址，对于串口，该值无效
**                      buffer: 读取数据存放地址
**                      size:       读取数据长度
** output parameters:   实际接收到的数据长度
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
** Descriptions:        从串口发送指定字节的数据，或者采用DMA发送，或者采用中断发送
** input parameters:    dev: 外设特性描述符
**                      pos:    数据偏移地址，对于串口，该值无效
**                      buffer: 发送数据存放地址
**                      size:       发送数据长度
** output parameters:   实际发送的数据长度
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
** Descriptions:        串口控制，可以是配置波特率，数据格式等
** input parameters:    dev:     外设特性描述符
**                      cmd:    配置命令
**                      args:     配置参数
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
      
      // 将ms转换为时钟tick
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
      
      // 将ms转换为时钟tick
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
** Descriptions:        gsmmux复位，会重新初始化，重新建立gsmmux逻辑通道
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
** Descriptions:        获取gsmmux的状态
** Input parameters:    NONE
** Output parameters:   NONE
** Returned value:      0：初始化中； 1：通道建立成功
*********************************************************************************************************/
uint8_t gsmmux_status(void)
{
   return __ggsmmux_status;
}

/*********************************************************************************************************
** Function name:       gsmmux_init
** Descriptions:        gsmmux初始化
** Input parameters:    name:       使用的串口
**                      baud:       串口波特率
**                      power_ctrl：modem电源控制
**                      enter_mux：进入mux模式控制
** Output parameters:   None 无
** Returned value:      -1: 指定的串口不存在
**                      -2: 内存不够
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
  ** Step 1, 初始化串口设备
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

     // 初始化控制通道
     __ggsmmux_serial_channel[0].gsm0710 = &__ggsmmux_0710_use;
     __ggsmmux_serial_channel[0].channel = 0;
     __ggsmmux_serial_channel[0].opened = 0;
     __ggsmmux_serial_channel[0].v24_signals = 0;
     __ggsmmux_serial_channel[0].RecvTimeoutSave = 0;
     __ggsmmux_serial_channel[0].RecvIntervalTimeSave = 0;
      gsmmux_buffer_init(&(__ggsmmux_serial_channel[0].serial_buffer), RT_NULL, 0);
     
     // 初始化ppp逻辑通道结构
     __ggsmmux_serial_channel[1].gsm0710 = &__ggsmmux_0710_use;
     __ggsmmux_serial_channel[1].channel = 1;
     __ggsmmux_serial_channel[1].opened = 0;
     __ggsmmux_serial_channel[1].v24_signals = 0;
     __ggsmmux_serial_channel[1].RecvTimeoutSave = 0;
     __ggsmmux_serial_channel[1].RecvIntervalTimeSave = 0;
     gsmmux_buffer_init(&(__ggsmmux_serial_channel[1].serial_buffer), gsm_channel_1_buffer, 4096);
     
     // 初始化其它逻辑通道缓冲区
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
  ** Step 2, 注册操作系统虚拟设备
  */
  for(i=0; i<GSM0710_PORT_NUM; i++) 
  {
    sprintf(mux_name, "gsmmux%d", i+1);
    handle = rt_device_find(mux_name);
    // 如果不存在，则注册设备
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
  ** Step 3, 创建接收线程
  */
  tid = rt_thread_find("muxrecv");
  // 如果不存在，则创建线程
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
