/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           ttyuart_win32.c
** Last modified Date:  2018-09-19
** Last Version:        v1.00
** Description:         windows�µ�������������
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2018-09-19
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
#include <rtlibc.h>

#include <windows.h>

/*********************************************************************************************************
** ��������궨��
*********************************************************************************************************/
#define WIN32_TTYUART_DEBUG    0

#if (WIN32_TTYUART_DEBUG > 0)
#define win32_ttyuart_trace(fmt, ...)           printf(fmt, ##__VA_ARGS__)
#else
#define win32_ttyuart_trace(fmt, ...)
#endif

/*********************************************************************************************************
** ��Ҫ�ĺ궨��
*********************************************************************************************************/
#define TTY_UART_BUFFER_POOL_CNT                 64      // ����������
#define TTY_UART_BUFFER_SIZE                     1024   // ÿ����������С

/*********************************************************************************************************
** �����Ƿ�����������ݶ��еĺ�
*********************************************************************************************************/
#define Serial_Buffer_Empty(a)    \
(((a->buf_read) == (a->buf_write)) ? RT_TRUE : RT_FALSE)

#define Serial_Buffer_Full(a)     \
((((a->buf_write + 1) % TTY_UART_BUFFER_POOL_CNT) == (a->buf_read)) ? RT_TRUE : RT_FALSE)

/*********************************************************************************************************
** �����ṹ��ض���
*********************************************************************************************************/
struct tty_recv_buffer{
    uint8_t buffer[TTY_UART_BUFFER_SIZE];
    uint16_t size;
};
struct ttyuart_device
{
    const char *win_com;                                                    // win�µ��豸
    HANDLE fd;                                                              // ���ھ��
    rt_mutex_t   send_mutex;                                                // ���̷߳��ͱ���
    rt_mutex_t   recv_mutex;                                                // ���߳̽��ձ���
    struct rt_completion Rx_Comp;                                           // ������������
    struct tty_recv_buffer recv_buffer[TTY_UART_BUFFER_POOL_CNT];           // ���ջ�����
    uint16_t buf_read;                                                      // ���ж�ָ��
    uint16_t buf_write;                                                     // ����дָ��
    uint32_t RecvTimeoutSave;                                               //  ���ճ�ʱ���������棬�������ñ���
    uint32_t RecvIntervalTimeSave;                                          //  ����ʱ����
    struct serial_configure serial_configure;                               //  ��ǰ��������
};

/*********************************************************************************************************
** Function name:       tty_uart_recv_thread
** Descriptions:        ���ڽ��մ���win�߳�
** input parameters:    lParam: �̴߳������
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static DWORD WINAPI tty_uart_recv_thread(LPVOID lParam)
{
    struct ttyuart_device *serial_handle = lParam;
    HANDLE hCom = serial_handle->fd;
    BOOL bReadStat;
    DWORD dwCount;
    DWORD dwErrorFlags;
    COMSTAT ComStat;
    uint8_t *recv_data;

    for(;;)
    {
        ClearCommError(serial_handle->fd,&dwErrorFlags,&ComStat);
        if(Serial_Buffer_Full(serial_handle)) {
            rt_completion_done(&serial_handle->Rx_Comp);
            //PurgeComm(hCom, /*PURGE_TXCLEAR|*/PURGE_RXCLEAR);
            Sleep(1);
            continue;
        }

        recv_data = serial_handle->recv_buffer[serial_handle->buf_write].buffer;
        //ClearCommError(hCom,&dwErrorFlags,&ComStat);
        dwCount = TTY_UART_BUFFER_SIZE;
        bReadStat=ReadFile(hCom, recv_data, dwCount, &dwCount, NULL);
        if(!bReadStat)
        {
            Sleep(10);
            continue;
        }

        if(dwCount <= 0)
        {
            Sleep(10);
            continue;
        }
        serial_handle->recv_buffer[serial_handle->buf_write].size = dwCount;
        serial_handle->buf_write = (serial_handle->buf_write + 1) % TTY_UART_BUFFER_POOL_CNT;
        rt_completion_done(&serial_handle->Rx_Comp);
    }
}

/*********************************************************************************************************
** Function name:       win32_serial_init
** Descriptions:        ��׼�豸����init
** input parameters:    dev: �豸�����ṹ
** output parameters:   NONE
** Returned value:      RT_EOK: ���ڳ�ʼ���ɹ�;  ����, ���ڳ�ʼ��ʧ��
*********************************************************************************************************/
static rt_err_t win32_serial_init (rt_device_t dev)
{
    struct ttyuart_device *serial_handle = dev->user_data;

    RT_ASSERT(dev != RT_NULL);

    // ��ʼ�����ͱ�������
    serial_handle->send_mutex = rt_mutex_create("txMutex", RT_IPC_FLAG_FIFO);
    // ��ʼ�����ձ�������
    serial_handle->recv_mutex = rt_mutex_create("rxMutex", RT_IPC_FLAG_FIFO);
    // ��ʼ����������
    rt_completion_init(&serial_handle->Rx_Comp);

    serial_handle->fd = CreateFile(serial_handle->win_com, // ָ���򿪵Ĵ���
                     GENERIC_READ | GENERIC_WRITE,      //   ��д��ʽ
                     0,                                 //  ��ռ��ʽ��
                     NULL,
                     OPEN_EXISTING,
                     0, //FILE_FLAG_OVERLAPPED, //0,                                 //  ͬ����ʽ�����첽��ʽ
                     NULL);
    if((HANDLE)-1 == serial_handle->fd)
    {
        printf("Open win32 %s error\r\n", serial_handle->win_com);
        return -RT_EIO;
    }

    serial_handle->RecvTimeoutSave = 5000;
    serial_handle->RecvIntervalTimeSave = 5;
    serial_handle->serial_configure.baud_rate = 115200;
    serial_handle->serial_configure.data_bits = DATA_BITS_8;
    serial_handle->serial_configure.parity = PARITY_NONE;
    serial_handle->serial_configure.stop_bits = STOP_BITS_1;

    SetupComm(serial_handle->fd,2048*1024,2048*1024);
    COMMTIMEOUTS TimeOuts;
    TimeOuts.ReadIntervalTimeout =  5;  // MAXDWORD;
    TimeOuts.ReadTotalTimeoutMultiplier = 0;
    TimeOuts.ReadTotalTimeoutConstant = 50;
    TimeOuts.WriteTotalTimeoutMultiplier = 0;
    TimeOuts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(serial_handle->fd,&TimeOuts);
    DCB dcb;
    GetCommState(serial_handle->fd,&dcb);
    dcb.BaudRate = 115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(serial_handle->fd,&dcb);
    PurgeComm(serial_handle->fd, PURGE_TXCLEAR|PURGE_RXCLEAR);

    CreateThread(NULL,0,tty_uart_recv_thread,serial_handle,0,0);
    return RT_EOK;
}

/*********************************************************************************************************
** Function name:       win32_serial_open
** Descriptions:        �豸����open
** input parameters:    dev: �豸�����ṹ
** output parameters:   NONE
** Returned value:      RT_EOK: ���ڳ�ʼ���ɹ�;  ����, ���ڳ�ʼ��ʧ��
*********************************************************************************************************/
static rt_err_t win32_serial_open(rt_device_t dev, rt_uint16_t oflag)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       win32_serial_close
** Descriptions:        �豸����close
** input parameters:    dev: �豸�����ṹ
** output parameters:   NONE
** Returned value:      RT_EOK: ���ڳ�ʼ���ɹ�;  ����, ���ڳ�ʼ��ʧ��
*********************************************************************************************************/
static rt_err_t win32_serial_close(rt_device_t dev)
{
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       win32_serial_read
** Descriptions:        �豸����read
** input parameters:    dev: ��������������
**                      pos:    ����ƫ�Ƶ�ַ�����ڴ��ڣ���ֵ��Ч
**                      buffer: ��ȡ���ݴ�ŵ�ַ
**                      size:   ��ȡ���ݳ���
** output parameters:   ʵ�ʽ��յ������ݳ���
*********************************************************************************************************/
static rt_size_t win32_serial_read (rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    struct ttyuart_device *serial_handle = dev->user_data;


    rt_mutex_take(serial_handle->recv_mutex, RT_WAITING_FOREVER);
    rt_completion_wait(&serial_handle->Rx_Comp,RT_WAITING_FOREVER);
    if(Serial_Buffer_Empty(serial_handle))
    {
        size = 0;
    } else {
        size = serial_handle->recv_buffer[serial_handle->buf_read].size;
        memcpy(buffer, serial_handle->recv_buffer[serial_handle->buf_read].buffer, size);
        serial_handle->buf_read = (serial_handle->buf_read + 1) % TTY_UART_BUFFER_POOL_CNT;
        if(!(Serial_Buffer_Empty(serial_handle))) {
            rt_completion_done(&serial_handle->Rx_Comp);
        }
    }
    rt_mutex_release(serial_handle->recv_mutex);

    return size;
}

/*********************************************************************************************************
** Function name:       win32_serial_write
** Descriptions:        �豸����write
** input parameters:    dev: ��������������
**                      pos:    ����ƫ�Ƶ�ַ�����ڴ��ڣ���ֵ��Ч
**                      buffer: �������ݴ�ŵ�ַ
**                      size:   �������ݳ���
** output parameters:   ʵ�ʷ��͵����ݳ���
*********************************************************************************************************/
static rt_size_t win32_serial_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    struct ttyuart_device *serial_handle = dev->user_data;
    BOOL bWriteStat;
    DWORD dwBytesWrite;
    DWORD dwErrorFlags;
    COMSTAT ComStat;

    rt_mutex_take(serial_handle->send_mutex, RT_WAITING_FOREVER);
    dwBytesWrite = size;
    ClearCommError(serial_handle->fd,&dwErrorFlags,&ComStat);
    bWriteStat = WriteFile(serial_handle->fd,buffer,dwBytesWrite,&dwBytesWrite,NULL);
    if(!bWriteStat)
    {
        ;
    }
    rt_mutex_release(serial_handle->send_mutex);
    return dwBytesWrite;
}

/*********************************************************************************************************
** Function name:       win32_serial_control
** Descriptions:        �豸����control
** input parameters:    dev:    ��������������
**                      cmd:    ��������
**                      args:   ���ò���
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_err_t win32_serial_control (rt_device_t dev, rt_uint8_t cmd, void *args)
{
}

/*********************************************************************************************************
** ���������ṹ��֧���������豸������
*********************************************************************************************************/
static struct ttyuart_device __gwin32_uart_info =
{
    "\\\\.\\COM2",          // win�µ��豸
    NULL,                   // ���ھ��
    RT_NULL,                // ���̷߳��ͱ���
    RT_NULL,                // ���߳̽��ձ���
    {0},                    // ������������
    {0},                    // ���ջ�����
    0,                      // ���ж�ָ��
    0,                      // ����дָ��
    0,                      //  ���ճ�ʱ���������棬�������ñ���
    0,                      //  ����ʱ����
    {0}                     //  ��ǰ��������
};
static struct rt_device __gwin32_uart_device;

/*********************************************************************************************************
** Function name:       rt_win32_ttyuart_driver_init
** Descriptions:        ����������ʼ����������Ҫ����ע���ж�
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void rt_win32_ttyuart_driver_init(void)
{
#if 0
  rt_device_t device;

  // ע������
  device = &__gwin32_uart_device;
  device->type      = RT_Device_Class_Char;
  device->rx_indicate = RT_NULL;
  device->tx_complete = RT_NULL;
  device->init      = win32_serial_init;
  device->open      = win32_serial_open;
  device->close     = win32_serial_close;
  device->read      = win32_serial_read;
  device->write     = win32_serial_write;
  device->control   = win32_serial_control;
  device->user_data = &__gwin32_uart_info;

  /* register a character device */
  rt_device_register(device, "ttyS0", RT_DEVICE_FLAG_RDWR);
  #endif
}

/*********************************************************************************************************
** �����Զ���ʼ������
*********************************************************************************************************/
INIT_DEVICE_EXPORT(rt_win32_ttyuart_driver_init);

/*********************************************************************************************************
END FILE
*********************************************************************************************************/

