/****************************************Copyright (c)****************************************************
**                             成 都 冷 云 能 源 科 技 有 限 公 司
**                                   https://www.bmofang.com/
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           modem_app.c
** Last modified Date:  2014-04-09
** Last Version:        v1.0
** Description:         modem应用
**
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo任海波
** Created date:        2014-04-09
** Version:             v1.0
** Descriptions:        The original version 初始版本
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
**
*********************************************************************************************************/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <dfs_posix.h>
#include <sys/time.h>
#include <dfs_select.h>

/*********************************************************************************************************
** Function name:       pipe_read
** Descriptions:        pipe异步IO读数据
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int pipe_read(int fd, char *buffer, int len, int timeout)
{
  fd_set fds;
  rt_int32_t max_fd = 0, res = 0;
  struct timeval timeout_val;

  max_fd = fd + 1;
  FD_ZERO(&fds);

  FD_SET(fd, &fds);

  timeout_val.tv_sec = timeout;
  timeout_val.tv_usec = 0;
  res = select(max_fd, &fds, RT_NULL, RT_NULL, &timeout_val);

  /* data is ready */
  if (FD_ISSET(fd, &fds))
  {
    return read(fd, buffer, len);
  }

  return -1;
}

/*********************************************************************************************************
** Function name:       pipe_test
** Descriptions:        pipe测试
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static uint8_t pipe_buffer[1024];
int pipe_test(int argc, char **argv)
{
  char dev_name[32];
  rt_pipe_t *pipe = RT_NULL;
  int read_fd, write_fd;
  int len;

  if(argc != 1) {
    rt_kprintf("mustn't have param\r\n");
  }

  pipe = rt_pipe_create("pipe0", 1024 /*PIPE_BUFSZ*/);
  if (pipe == RT_NULL)
  {
    rt_kprintf("pipe create failed\n");
    return -1;

  }

  snprintf(dev_name, sizeof(dev_name), "/dev/pipe0");
  read_fd = open(dev_name, O_RDONLY, 0);
  if (read_fd < 0) {
    len = -1;
    goto fail_read;
  }


  write_fd = open(dev_name, O_WRONLY, 0);
  if (write_fd < 0) {
    len = -1;
    goto fail_write;
  }

  rt_kprintf("pipe init succeed\n");

  write(write_fd, pipe_buffer, 1024);

  // 这里会阻塞线程，必须用select实现超时机制
  len = pipe_read(read_fd, (char *)pipe_buffer, 1024, 3);
  if(len > 0) {
    rt_kprintf("pipe0 recv data length %d\r\n", len);
  } else {
    rt_kprintf("pipe0 can't recv data\r\n");
  }
  close(write_fd);

fail_write:
  close(read_fd);

fail_read:
    rt_pipe_delete("pipe0");
    return len;
}

#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(pipe_test,  pipe test example);
#endif /* FINSH_USING_MSH */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
