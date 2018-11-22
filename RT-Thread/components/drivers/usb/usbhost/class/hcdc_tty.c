/*
* Copyright (c) 2006-2018, RT-Thread Development Team
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date           Author       Notes
* 2018-11-01     hichard      first version
*/
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "hcdc.h"

#ifdef RT_USBH_CDC_TTY

/**
* This function realize the device driver init
*
* @param dev the device driver struct.
* 
* @return the error code, RT_EOK on successfully.
*/
static rt_err_t _rt_ttyusb_init(rt_device_t dev)
{
  return RT_EOK;
}

/**
* This function realize the device driver open
*
* @param dev the device driver struct.
* @param oflag open flag, it is not valid here. 
* 
* @return the error code, RT_EOK on successfully.
*/
static rt_err_t _rt_ttyusb_open(rt_device_t dev, rt_uint16_t oflag)
{
  return RT_EOK;
}

/**
* This function realize the device driver close
*
* @param dev the device driver struct. 
* 
* @return the error code, RT_EOK on successfully.
*/
static rt_err_t _rt_ttyusb_close(rt_device_t dev)
{
  return RT_EOK;
}

/**
* This function realize the device driver read
*
* @param dev the device driver struct. 
* 
* @return the error code, RT_EOK on successfully.
*/
static rt_size_t _rt_ttyusb_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
  cdc_intf_t *cdc = (cdc_intf_t *)dev->user_data;
  
  RT_ASSERT(dev != RT_NULL);
  
  if(cdc->status == RT_FALSE) {
    rt_set_errno(-ENXIO);
    return 0;
  }
  
  return rt_usb_hcd_pipe_xfer(cdc->pipe_in->inst->hcd, cdc->pipe_in, buffer, size, cdc->timeout);
}

/**
* This function realize the device driver write
*
* @param dev the device driver struct. 
* 
* @return the error code, RT_EOK on successfully.
*/
static rt_size_t _rt_ttyusb_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
  cdc_intf_t *cdc = (cdc_intf_t *)dev->user_data;
  
  RT_ASSERT(dev != RT_NULL);
  
  if(cdc->status == RT_FALSE) {
    rt_set_errno(-ENXIO);
    return 0;
  }
  
  return rt_usb_hcd_pipe_xfer(cdc->pipe_out->inst->hcd, cdc->pipe_out, (void *)buffer, size, cdc->timeout);
}

/**
* This function realize the device driver control
*
* @param dev the device driver struct. 
* 
* @return the error code, RT_EOK on successfully.
*/

static rt_err_t _rt_ttyusb_control (rt_device_t dev, int cmd, void *args)
{
  rt_err_t err_code = RT_EOK;
  rt_base_t level;
  cdc_intf_t *cdc = (cdc_intf_t *)dev->user_data;
  
  RT_ASSERT(dev != RT_NULL);
  
  switch (cmd)
  {
  case RT_DEVICE_CTRL_SUSPEND:
    {
      /* suspend device */
      dev->flag |= RT_DEVICE_FLAG_SUSPENDED;
      break;
    }
  case RT_DEVICE_CTRL_RESUME:
    {
      /* resume device */
      dev->flag &= ~RT_DEVICE_FLAG_SUSPENDED;
      break;
    }
  case RT_DEVICE_CTRL_CONFIG_SET:
    {
      struct serial_configure *pconfig = args;
      if(args == RT_NULL) {
        break;
      }
      /* disable interrupt */
      level = rt_hw_interrupt_disable();
      cdc->config = *pconfig;
      /* enable interrupt */
      rt_hw_interrupt_enable(level);
      break;
    }
  case RT_DEVICE_CTRL_CONFIG_GET:
    {
      struct serial_configure *pconfig = args;
      
      if(args == RT_NULL) {
        err_code = RT_ERROR;
        break;
      }
      
      /* disable interrupt */
      level = rt_hw_interrupt_disable();
      *pconfig = cdc->config;
      /* enable interrupt */
      rt_hw_interrupt_enable(level);
      break;
    }
  case RT_DEVICE_CRTL_TIMEOUT:
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
      cdc->timeout = count;
      /* enable interrupt */
      rt_hw_interrupt_enable(level);
      break;
    }
  case RT_DEVICE_CTRL_INTERVAL:
    {
      break;
    }
  default:
    err_code = RT_ERROR;
    break;
  }
  
  return err_code;
}

/**
* This function realize the device driver register
*
* @param dev the device driver struct. 
* 
* @return none
*/
void rt_ttyusb_register(cdc_intf_t *cdc)
{
  char name[RT_NAME_MAX];
    
  cdc->tty_dev.type        = RT_Device_Class_Char;
  cdc->tty_dev.rx_indicate = RT_NULL;
  cdc->tty_dev.tx_complete = RT_NULL;
  cdc->tty_dev.init        = _rt_ttyusb_init;
  cdc->tty_dev.open        = _rt_ttyusb_open;
  cdc->tty_dev.close       = _rt_ttyusb_close;
  cdc->tty_dev.read        = _rt_ttyusb_read;
  cdc->tty_dev.write       = _rt_ttyusb_write;
  cdc->tty_dev.control     = _rt_ttyusb_control;
  cdc->tty_dev.user_data   = cdc;
  
  /* register a character device */
  rt_snprintf(name, RT_NAME_MAX, "ttyUSB%d", cdc->tty_number);
  rt_device_register(&cdc->tty_dev, name, RT_DEVICE_FLAG_RDWR);
}

/**
* This function realize the device driver unregister
*
* @param dev the device driver struct. 
* 
* @return none
*/
void rt_ttyusb_unregister(cdc_intf_t *cdc)
{
  rt_device_unregister(&cdc->tty_dev);
}

/**
* This function realize the device driver init
*
* @param dev the device driver struct. 
* 
* @return the error code, RT_EOK on successfully.
*/
int rt_ttyusb_driver_init(void)
{
#ifdef RT_USE_TTYUSB0
  {
    static cdc_intf_t ttyusb0;
    
    ttyusb0.interface = 0;
    ttyusb0.tty_number = 0;
    ttyusb0.status = RT_FALSE;
    ttyusb0.timeout = 0;
    
    rt_usbh_cdc_interface_register(&ttyusb0);
  }
#endif 
  
#ifdef RT_USE_TTYUSB1
  {
    static cdc_intf_t ttyusb1;
    
    ttyusb1.interface = 1;
    ttyusb1.tty_number = 1;
    ttyusb1.status = RT_FALSE;
    ttyusb1.timeout = 0;
    
    rt_usbh_cdc_interface_register(&ttyusb1);
  }
#endif 
  
#ifdef RT_USE_TTYUSB2
  {
    static cdc_intf_t ttyusb2;
    
    ttyusb2.interface = 2;
    ttyusb2.tty_number = 2;
    ttyusb2.status = RT_FALSE;
    ttyusb2.timeout = 0;
    
    rt_usbh_cdc_interface_register(&ttyusb2);
  }
#endif

#ifdef RT_USE_TTYUSB3
  {
    static cdc_intf_t ttyusb3;
    
    ttyusb3.interface = 3;
    ttyusb3.tty_number = 3;
    ttyusb3.status = RT_FALSE;
    ttyusb3.timeout = 0;
    
    rt_usbh_cdc_interface_register(&ttyusb3);
  }
#endif
  
#ifdef RT_USE_TTYUSB4
  {
    static cdc_intf_t ttyusb4;
    
    ttyusb4.interface = 4;
    ttyusb4.tty_number = 4;
    ttyusb4.status = RT_FALSE;
    ttyusb4.timeout = 0;
    
    rt_usbh_cdc_interface_register(&ttyusb4);
  }
#endif 
  
#ifdef RT_USE_TTYUSB5
  {
    static cdc_intf_t ttyusb5;
    
    ttyusb5.interface = 5;
    ttyusb5.tty_number = 5;
    ttyusb5.status = RT_FALSE;
    ttyusb5.timeout = 0;
    
    rt_usbh_cdc_interface_register(&ttyusb5);
  }
#endif 
  
 return 0;
}

INIT_ENV_EXPORT(rt_ttyusb_driver_init);
#endif
