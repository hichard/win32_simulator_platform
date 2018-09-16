/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           gsmmux_buffer.c
** Last modified Date:  2017-11-11
** Last Version:        v1.00
** Description:         gsmmux实现内存管理
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

#include "gsmmux_opts.h"
#include "gsmmux_buffer.h"

/*********************************************************************************************************
** 调试输出宏定义
*********************************************************************************************************/
#if (GSMMUX_BUFFER_DEBUG > 0)
#define gsmmux_buffer_trace(fmt, ...)           rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define gsmmux_buffer_trace(fmt, ...)
#endif

/*********************************************************************************************************
** 宏定义，查看缓冲区使用了多少,即当前存储了多少数据
*********************************************************************************************************/
#define __buffer_used(a) \
((a->writep >= a->readp) ? (a->writep - a->readp) : (a->size + a->writep - a->readp))

/*********************************************************************************************************
** 宏定义，查看缓冲区剩余多少
*********************************************************************************************************/
#define __buffer_free(a) \
(((a->readp > a->writep) ? (a->readp - a->writep) : (a->size + a->readp - a->writep)) - 1)

/*********************************************************************************************************
** GSM0710 CRC8计算的查询表格 (reversed, 8-bit, poly=0x07)
*********************************************************************************************************/
static const uint8_t gsm_crctable[256] = 
{ 
  0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 
  0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B, 
  0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 
  0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67, 
  0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 
  0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43, 
  0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 
  0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F, 
  0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 
  0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B, 
  0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 
  0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17, 
  0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 
  0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33, 
  0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 
  0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F, 
  0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 
  0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B, 
  0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 
  0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87, 
  0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 
  0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3, 
  0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 
  0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF, 
  0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 
  0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB, 
  0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 
  0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7, 
  0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 
  0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3, 
  0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 
  0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF 
};

/*********************************************************************************************************
** Function name:       gsmmux_buffer_lock
** Descriptions:        指定缓冲区加锁
** Input parameters:    gsm_buffer:指定初始化的缓冲区
** Output parameters:   None 无
** Returned value:      None 无
*********************************************************************************************************/
static void gsmmux_buffer_lock(gsmmux_buffer_t *gsm_buffer)
{
  rt_mutex_take(&gsm_buffer->lock, RT_WAITING_FOREVER);
}

/*********************************************************************************************************
** Function name:       gsmmux_buffer_unlock
** Descriptions:        指定缓冲区解锁
** Input parameters:    gsm_buffer:指定初始化的缓冲区
** Output parameters:   None 无
** Returned value:      None 无
*********************************************************************************************************/
static void gsmmux_buffer_unlock(gsmmux_buffer_t *gsm_buffer)
{
  rt_mutex_release(&gsm_buffer->lock);
}

/*********************************************************************************************************
** Function name:       gsmmux_make_fcs
** Descriptions:        gsmmux校验计算
** Input parameters:    input: 待计算的缓冲区
**                      count: 待计算的数据大小
**                      crc8:  校验初值
** Output parameters:   None 无
** Returned value:      校验结果
*********************************************************************************************************/
uint8_t gsmmux_make_fcs(const uint8_t *input, uint32_t length, uint8_t crc8) 
{
  uint32_t i;
  
  crc8 ^= 0xFF;
  for (i = 0; i < length; i++)
  {
    crc8 = gsm_crctable[crc8 ^ input[i]];
  }
  
  return crc8 ^ 0xFF;
}

/*********************************************************************************************************
** Function name:       gsmmux_buffer_write
** Descriptions:        写缓冲区
** Input parameters:    gsm_buffer:指定初始化的缓冲区
**                      buffer:    存储缓冲区
**                      size：     缓冲区大小
** Output parameters:   None 无
** Returned value:      实际写入的数据长度
*********************************************************************************************************/
uint16_t gsmmux_buffer_write(gsmmux_buffer_t *gsm_buffer, const uint8_t *buffer, uint16_t length)
{
    uint16_t free;
    uint16_t last;

    gsmmux_buffer_lock(gsm_buffer);
    if(length > (gsm_buffer->size - 1))
    {
       length = gsm_buffer->size - 1;
    }
    
    free = __buffer_free(gsm_buffer);
    if(length > free) 
    {
       gsmmux_buffer_trace("gsm buffer write overflow!\r\n");
       length = free;
    }

    if (length > 0)
    {
        last = gsm_buffer->size - gsm_buffer->writep;
        if(last >= length)
        {
            memcpy(&gsm_buffer->buffer[gsm_buffer->writep], buffer, length);
        }
        else
        {
            memcpy(&gsm_buffer->buffer[gsm_buffer->writep], buffer, last);
            memcpy(gsm_buffer->buffer, &buffer[last], length - last);
        }
        gsm_buffer->writep += length;
        if (gsm_buffer->writep >= gsm_buffer->size)
        {
            gsm_buffer->writep -= gsm_buffer->size;
        }
    }
    // 如果有数据，则释放信号
    if(__buffer_used(gsm_buffer) != 0) {
      rt_completion_done(&gsm_buffer->Comp);
    }
    gsmmux_buffer_unlock(gsm_buffer);

    return length;
}

/*********************************************************************************************************
** Function name:       gsmmux_buffer_read
** Descriptions:        读缓冲区
** Input parameters:    gsm_buffer:指定初始化的缓冲区
**                      buffer:    存储缓冲区
**                      size：     缓冲区大小
**                      timeout:   无数据收到时的超时退出时间
**                      interval:  有数据收到间隔超时时间
** Output parameters:   None 无
** Returned value:      None 无
*********************************************************************************************************/
uint16_t gsmmux_buffer_read(gsmmux_buffer_t *gsm_buffer, 
                            uint8_t *buffer, uint16_t size, uint32_t timeout, uint32_t interval)
{
    uint16_t used;
    uint16_t last;
    uint16_t length = 0;
    int32_t  wait_time;

    for(;;)
    {
        gsmmux_buffer_lock(gsm_buffer);
        used = __buffer_used(gsm_buffer);

        if (used > size - length)
        {
            used = size - length;
        }
        if (used > 0)
        {
            last = gsm_buffer->size - gsm_buffer->readp;
            if(last >= used)
            {
                memcpy(buffer, &gsm_buffer->buffer[gsm_buffer->readp], used);
            }
            else
            {
                memcpy(buffer, &gsm_buffer->buffer[gsm_buffer->readp], last);
                memcpy(&buffer[last], gsm_buffer->buffer, used - last);
            }
            gsm_buffer->readp += used;
            if (gsm_buffer->readp >= gsm_buffer->size)
            {
                gsm_buffer->readp -= gsm_buffer->size;
            }
            length += used;
            if (length >= size)
            {
                gsmmux_buffer_unlock(gsm_buffer);
                break;
            }
        }
        
        // 如果有数据，则释放信号
        if(__buffer_used(gsm_buffer) != 0) {
          rt_completion_done(&gsm_buffer->Comp);
        } else {
          rt_completion_init(&gsm_buffer->Comp);
        }
        gsmmux_buffer_unlock(gsm_buffer);

        if(length == 0) {
          if(timeout == 0) {
             wait_time = RT_WAITING_FOREVER;
          } else {
            wait_time = timeout;
          }
        } else {
          wait_time = interval;
        }
        
        if(rt_completion_wait(&gsm_buffer->Comp,wait_time) != RT_EOK) {
          break;
        }
    }

    return length;
}

/*********************************************************************************************************
** Function name:       gsmmux_buffer_init
** Descriptions:        初始化指定的缓冲区
** Input parameters:    gsm_buffer:指定初始化的缓冲区
**                      buffer:    存储缓冲区
**                      size：     缓冲区大小
** Output parameters:   None 无
** Returned value:      None 无
*********************************************************************************************************/
void gsmmux_buffer_init(gsmmux_buffer_t *gsm_buffer, uint8_t *buffer, uint16_t size)
{
  memset(gsm_buffer, 0, sizeof(gsmmux_buffer_t));
  
  rt_mutex_init(&gsm_buffer->lock, "gsmlock", RT_IPC_FLAG_FIFO);
  rt_completion_init(&gsm_buffer->Comp);
  
  gsm_buffer->buffer = buffer;
  gsm_buffer->size = size; 
}

/*********************************************************************************************************
END FILE
*********************************************************************************************************/
