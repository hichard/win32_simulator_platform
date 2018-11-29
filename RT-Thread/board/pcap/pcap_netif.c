/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           pcap_netif.c
** Last modified Date:  2018-11-29
** Last Version:        v1.00
** Description:         windows下的Winpcap虚拟网卡驱动
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2018-11-29
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

#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS
#include <lwip_if.h>
#include <lwip_ethernet.h>

#include <windows.h>
#include <pcap.h>

/*********************************************************************************************************
** 配置使用的网卡编号
*********************************************************************************************************/
#define WINPCAP_NET_USE_NO      1

/*********************************************************************************************************
** 需要的宏定义
*********************************************************************************************************/
#define MAX_ADDR_LEN        6

/*********************************************************************************************************
** 驱动结构相关定义
*********************************************************************************************************/
struct pcap_netif
{
	/* inherit from ethernet device */
	struct eth_device parent;

    pcap_t *tap;

	/* interface address info. */
	rt_uint8_t  dev_addr[MAX_ADDR_LEN];     /* hw address	*/
	struct rt_mutex lock;
	rt_mailbox_t packet_mb;
};

/*********************************************************************************************************
** 全局变量定义
*********************************************************************************************************/
static struct pcap_netif    pcap_netif_device;

/*********************************************************************************************************
** Function name:       pcap_thread_entry
** Descriptions:        接收数据包处理线程
** input parameters:    lParam: 线程传入参数
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static DWORD WINAPI pcap_thread_entry(LPVOID lParam)
{
    static char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *header;
    const u_char *pkt_data;
    int res;

    struct pcap_netif *pcap_netif = (struct pcap_netif *)lParam;

    /* Read the packets */
    while (1)
    {
        struct pbuf *p;

        rt_enter_critical();
        res = pcap_next_ex(pcap_netif->tap, &header, &pkt_data);
        rt_exit_critical();

        if (res == 0) {
            Sleep(10);
            continue;
        }


        p = pbuf_alloc(PBUF_LINK, header->len, PBUF_RAM);
        pbuf_take(p, pkt_data, header->len);

        #if 1
        /* send to packet mailbox */
        if(RT_EOK == rt_mb_send(pcap_netif->packet_mb, (rt_uint32_t)p)) {
            /* notify eth rx thread to receive packet */
            eth_device_ready(&pcap_netif->parent);
        }
        #else
        /* send to packet mailbox */
        rt_mb_send_wait(pcap_netif->packet_mb, (rt_uint32_t)p, RT_WAITING_FOREVER);
        /* notify eth rx thread to receive packet */
        eth_device_ready(&pcap_netif->parent);
        #endif
    }
}

/*********************************************************************************************************
** Function name:       pcap_netif_open
** Descriptions:        设备驱动open
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_err_t pcap_netif_init(rt_device_t dev)
{
    pcap_if_t *alldevs;
    pcap_if_t *d;
    pcap_if_t *use;
    pcap_t *tap;
    int inum, i=0;
    static char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_netif *pcap_netif = (struct pcap_netif *)dev->user_data;

    /* Retrieve the device list */
    if(pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        rt_kprintf("Error in pcap_findalldevs: %s\n", errbuf);
        return -RT_ERROR;
    }

    // 如果没有找到网卡
    if(alldevs == NULL) {
        rt_kprintf("\nNo interfaces found! Make sure WinPcap is installed.\n");
        return -RT_ERROR;
    }

    /* Print the list */
    use = NULL;
    for(d = alldevs; d; d = d->next)
    {
        rt_kprintf("%d. %s", ++i, d->name);
        if (d->description) {
            rt_kprintf(" (%s)\n", d->description);
        } else {
            rt_kprintf(" (No description available)\n");
        }
        if(i == WINPCAP_NET_USE_NO) {
            use = d;
        }
    }

    if(use == NULL) {
       rt_kprintf("The config net card is not exist\r\n");
       return -RT_ERROR;
    } else {
       rt_kprintf("The driver use %s as net card\r\n", use->description);
    }

     /* Open the adapter */
    if ((pcap_netif->tap = pcap_open_live(use->name,
        65536, // portion of the packet to capture.
        1,     // promiscuous mode (nonzero means promiscuous)
        1,     // read timeout, 0 blocked, -1 no timeout
        errbuf )) == NULL)
    {
        rt_kprintf("Unable to open the adapter. %s is not supported by WinPcap\n", use->name);
        return;
    }

    // 创建接受线程
    #if 1
    CreateThread(NULL,0,pcap_thread_entry,pcap_netif,0,0);
    #else
    {
        rt_thread_t tid;
        tid = rt_thread_create("pcap", pcap_thread_entry, pcap_netif,
                               2048, RT_THREAD_PRIORITY_MAX - 1, 10);
        if (tid != RT_NULL)
        {
            rt_thread_startup(tid);
        }
    }
    #endif

    pcap_freealldevs(alldevs);

    return RT_EOK;
}

/*********************************************************************************************************
** Function name:       pcap_netif_open
** Descriptions:        设备驱动open
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_err_t pcap_netif_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}

/*********************************************************************************************************
** Function name:       pcap_netif_close
** Descriptions:        设备驱动close
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_err_t pcap_netif_close(rt_device_t dev)
{
    struct pcap_netif *pcap_netif = (struct pcap_netif *)dev->user_data;

    pcap_close(pcap_netif->tap);
	return RT_EOK;
}

/*********************************************************************************************************
** Function name:       pcap_netif_read
** Descriptions:        设备驱动read
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_size_t pcap_netif_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	rt_set_errno(-RT_ENOSYS);
	return 0;
}

/*********************************************************************************************************
** Function name:       pcap_netif_write
** Descriptions:        设备驱动write
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_size_t pcap_netif_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
	rt_set_errno(-RT_ENOSYS);
	return 0;
}

/*********************************************************************************************************
** Function name:       pcap_netif_control
** Descriptions:        设备驱动control
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_err_t pcap_netif_control(rt_device_t dev, int cmd, void *args)
{
	switch (cmd)
	{
	case NIOCTL_GADDR:
		/* get mac address */
		if (args) rt_memcpy(args, pcap_netif_device.dev_addr, 6);
		else return -RT_ERROR;
		break;

	default :
		break;
	}

	return RT_EOK;
}

/*********************************************************************************************************
** Function name:       pcap_netif_tx
** Descriptions:        发送一个数据包
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
rt_err_t pcap_netif_tx( rt_device_t dev, struct pbuf* p)
{
	struct pbuf *q;
	rt_uint8_t *ptr;
    static rt_uint8_t buf[2048];
    rt_err_t result = RT_EOK;
    int res;
    struct pcap_netif *pcap_netif = (struct pcap_netif *)dev->user_data;

	/* lock EMAC device */
	rt_mutex_take(&pcap_netif->lock, RT_WAITING_FOREVER);

	/* copy data to tx buffer */
	q = p;
	ptr = (rt_uint8_t*)buf;
	while (q)
	{
		memcpy(ptr, q->payload, q->len);
		ptr += q->len;
		q = q->next;
	}

    rt_enter_critical();
    res = pcap_sendpacket(pcap_netif->tap, buf, p->tot_len);
    rt_exit_critical();

    if (res != 0)
    {
        rt_kprintf("Error sending the packet: \n", pcap_geterr(pcap_netif->tap));
        result = -RT_ERROR;
    }

	/* unlock EMAC device */
	rt_mutex_release(&pcap_netif->lock);

    return result;
}

/*********************************************************************************************************
** Function name:       pcap_netif_rx
** Descriptions:        接收一个数据包
** input parameters:    dev:   设备驱动结构
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct pbuf *pcap_netif_rx(rt_device_t dev)
{
	struct pbuf* p = RT_NULL;
    struct pcap_netif *pcap_netif = (struct pcap_netif *)dev->user_data;

    rt_mb_recv(pcap_netif->packet_mb, (rt_uint32_t*)&p, 0);

    return p;
}

/*********************************************************************************************************
** Function name:       pcap_netif_hw_init
** Descriptions:        pcap网卡注册
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void pcap_netif_hw_init(void)
{
	pcap_netif_device.dev_addr[0] = 0x00;
	pcap_netif_device.dev_addr[1] = 0x60;
	pcap_netif_device.dev_addr[2] = 0x37;
	/* set mac address: (only for test) */
	pcap_netif_device.dev_addr[3] = 0x12;
	pcap_netif_device.dev_addr[4] = 0x34;
	pcap_netif_device.dev_addr[5] = 0x56;

	rt_mutex_init(&pcap_netif_device.lock, "eth_lock", RT_IPC_FLAG_FIFO);
	pcap_netif_device.packet_mb = rt_mb_create("pcap", 64, RT_IPC_FLAG_FIFO);

	pcap_netif_device.parent.parent.init		= pcap_netif_init;
	pcap_netif_device.parent.parent.open		= pcap_netif_open;
	pcap_netif_device.parent.parent.close		= pcap_netif_close;
	pcap_netif_device.parent.parent.read		= pcap_netif_read;
	pcap_netif_device.parent.parent.write		= pcap_netif_write;
	pcap_netif_device.parent.parent.control	    = pcap_netif_control;
	pcap_netif_device.parent.parent.user_data	= &pcap_netif_device;

	pcap_netif_device.parent.eth_rx			= pcap_netif_rx;
	pcap_netif_device.parent.eth_tx			= pcap_netif_tx;

	eth_device_init(&(pcap_netif_device.parent), "e0");
}

#include <finsh.h>
void list_pcap(void)
{
    int i=0;
    pcap_if_t *alldevs;
    pcap_if_t *d;
    char errbuf[PCAP_ERRBUF_SIZE];

    /* Retrieve the device list */
    if(pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        rt_kprintf("Error in pcap_findalldevs: %s\n", errbuf);
        return -RT_ERROR;
    }

    /* Print the list */
    for(d = alldevs; d; d = d->next)
    {
        rt_kprintf("%d. %s", ++i, d->name);
        if (d->description)
            rt_kprintf(" (%s)\n", d->description);
        else
            rt_kprintf(" (No description available)\n");
    }
    if(i == 0)
    {
        rt_kprintf("\nNo interfaces found! Make sure WinPcap is installed.\n");
        return -RT_ERROR;
    }

    pcap_freealldevs(alldevs);

    return ;
}
#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(list_pcap,  show host netif adapter);
#endif /* FINSH_USING_MSH */
