/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           tcp_client_example.c
** Last modified Date:  2015-11-14
** Last Version:        v1.0
** Description:         TCP�ͻ�������
**
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo�κ���
** Created date:        2015-11-14
** Version:             v1.0
** Descriptions:        The original version ��ʼ�汾
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
** ȫ������
*********************************************************************************************************/
#define TCP_SERVER_ADDRESS    "192.168.100.13"
#define TCP_SERVER_PORT       "6000"

/*********************************************************************************************************
** Function name:       tcp_app_thread
** Descriptions:        tcp���ӳ���
** input parameters:    *parg
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
static uint8_t tcp_buffer[1024];
void tcp_app_thread(void *parg)
{
    int rv;
    int sockfd;
    int recv_len;
    struct addrinfo hints, *res, *p;
    //char ipstr[INET6_ADDRSTRLEN];
    char ipstr[40];

    /*
    ** Step 1, ��������
    */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version�� AF_UNSPEC is best
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(TCP_SERVER_ADDRESS, TCP_SERVER_PORT, &hints, &res)) != 0)
    {
        rt_kprintf( "getaddrinfo: get dns error\r\n");
        return;
    }
    for(p = res; p != NULL; p = p->ai_next)
    {
        void *addr;
        char *ipver;
        if (p->ai_family == AF_INET)   // IPv4
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
#if LWIP_IPV6
        else     // IPv6
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
#endif
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        rt_kprintf(" %s: %s\n", ipver, ipstr);
    }

    /*
    ** Step 2, ����socket,���ӷ�����
    */
    if ((sockfd = socket(res->ai_family, res->ai_socktype,
                         res->ai_protocol)) == -1)
    {
        rt_kprintf( "create socket error\r\n");
        freeaddrinfo(res); // free the linked list
        return;
    }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        closesocket(sockfd);
        rt_kprintf( "connect server error\r\n");
        freeaddrinfo(res); // free the linked list
        return;
    }

    /*
    ** Step 3, ���������շ�ѭ��
    */
    while (1)
    {
        recv_len = recv(sockfd, tcp_buffer, 1024, 0);
        if (recv_len == 0)
        {
            rt_kprintf("received error,close the socket.\r\n");
            break;
        }
        if(-1 == recv_len)
        {
            if(!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
            {
                rt_kprintf("received error,close the socket.\r\n");
                break;
            }
        }

        /* �������ݵ�sock���� */
        send(sockfd, tcp_buffer, recv_len, 0);
    }
    freeaddrinfo(res); // free the linked list
    closesocket(sockfd);
    return;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
int cmd_tcp_test(int argc, char **argv)
{
    rt_device_t tid;

    if (argc == 1)
    {
        tid = rt_thread_create("tcp", tcp_app_thread, RT_NULL, 1024, 10, 20);
        if (tid != RT_NULL) {
            rt_thread_startup(tid);
            rt_kprintf("create tcp test thread ok!\r\n");
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_tcp_test, __cmd_tcp_test, create tcp example thread);
#endif
/*********************************************************************************************************
** End of File
*********************************************************************************************************/
