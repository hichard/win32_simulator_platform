/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-30     hichard      first version
 */

#ifndef __HCDC_H__
#define __HCDC_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifndef USB_CLASS_CDC_CODE
#define USB_CLASS_CDC_CODE                       USB_CLASS_CDC
#endif

#define CDC_SET_LINE_CODING                     0x20
#define CDC_GET_LINE_CODING                     0x21
#define CDC_SET_CONTROL_LINE_STATE              0x22

/* wValue for SetControlLineState*/
#define CDC_ACTIVATE_CARRIER_SIGNAL_RTS         0x0002
#define CDC_DEACTIVATE_CARRIER_SIGNAL_RTS       0x0000
#define CDC_ACTIVATE_SIGNAL_DTR                 0x0001
#define CDC_DEACTIVATE_SIGNAL_DTR               0x0000

#pragma pack(1)
typedef struct {
    uint32_t             dwDTERate;     /*Data terminal rate, in bits per second*/
    uint8_t              bCharFormat;   /*Stop bits
    0 - 1 Stop bit
    1 - 1.5 Stop bits
    2 - 2 Stop bits*/
    uint8_t              bParityType;   /* Parity
    0 - None
    1 - Odd
    2 - Even
    3 - Mark
    4 - Space*/
    uint8_t              bDataBits;     /* Data bits (5, 6, 7, 8 or 16). */
} __cdc_line_coding_t;

typedef  union {
    __cdc_line_coding_t line_coding;
    uint8_t             buffer[7];
} cdc_line_coding_t;
#pragma pack()

typedef struct {
    rt_list_t   list;
    int         interface;        /* interface id */
    upipe_t     pipe_in;
    upipe_t     pipe_out; 
    rt_bool_t   status;
    
    int         tty_number;       /* ttyusb number */
    struct rt_device tty_dev;     /* rt_device struct */
    rt_uint32_t timeout;
    struct serial_configure config; 
} cdc_intf_t;

extern rt_err_t rt_usbh_cdc_class_init(void);
extern ucd_t rt_usbh_class_driver_cdc(void);
extern cdc_intf_t *rt_usbh_cdc_port_find(int protocal);
extern rt_err_t rt_usbh_cdc_interface_register(cdc_intf_t *cdc_intf);
extern rt_err_t rt_usbh_cdc_set_control_line_state(struct uhintf *intf, 
                                                   uint32_t wValue);
extern rt_err_t rt_usbh_cdc_set_line_code(struct uhintf *intf, 
                                          cdc_line_coding_t *line_coding);
extern rt_err_t rt_usbh_cdc_get_line_code(struct uhintf *intf, cdc_line_coding_t *line_coding);

#endif

