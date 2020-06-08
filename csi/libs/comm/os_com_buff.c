#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "os_com_buff.h"

#if !OS_COM_BUFF_MALLOC
static os_com_buff_t g_com_rx_buff;
static os_com_buff_t g_com_tx_buff;
#endif

os_com_buff_t *g_com_rx_buff_handle = NULL;
os_com_buff_t *g_com_tx_buff_handle = NULL;

void
os_com_buff_init (void)
{
#if !OS_COM_BUFF_MALLOC
  g_com_rx_buff_handle = &g_com_rx_buff;
  g_com_tx_buff_handle = &g_com_tx_buff;
#else
  g_com_rx_buff_handle = (os_com_buff_t *) malloc (sizeof (os_com_buff_t));
  if (g_com_rx_buff_handle == NULL)
    {
      /* TODO: Report error, Memory is not enough.  */
      return;
    }
  g_com_tx_buff_handle = (os_com_buff_t *) malloc (sizeof (os_com_buff_t));
  if (g_com_tx_buff_handle == NULL)
    {
      /* TODO: Report error, Memory is not enough.  */
      return;
    }
#endif
  memset (g_com_rx_buff_handle, 0, sizeof (os_com_buff_t));
  memset (g_com_tx_buff_handle, 0, sizeof (os_com_buff_t));

  return;
}

int
os_com_buff_read (os_com_buff_t *pbuff, uint8_t *ptr, int32_t length)
{
  int32_t       len = 0;
  if (pbuff == NULL || ptr == NULL|| length == 0)
    {
      /* Handle is not legal or read none data  */
      return 0;
    }

  while (length--)
    {
      if (pbuff->status == OS_COM_BUFF_STATUS_IDLE
          && pbuff->rx_idx == pbuff->tx_idx)
        break;

      *ptr++ = pbuff->buff[pbuff->rx_idx++];

      /* Cycle Buffer.  */
      pbuff->rx_idx = pbuff->rx_idx % OS_COM_BUFF_SIZE;

      len++;
      if (pbuff->rx_idx == pbuff->tx_idx
          && pbuff->status != OS_COM_BUFF_STATUS_IDLE)
        {
          pbuff->status = OS_COM_BUFF_STATUS_IDLE;
        }
    }
  return len;
}

int
os_com_buff_write (os_com_buff_t *pbuff, uint8_t *ptr, int32_t length)
{
  int32_t       len = 0;
  if (pbuff == NULL || ptr == NULL|| length == 0)
    {
      /* Handle is not legal or read none data  */
      return 0;
    }

  while (length--)
    {
      pbuff->buff[pbuff->tx_idx++] = *ptr++;

      /* Cycle Buffer.  */
      pbuff->tx_idx = pbuff->tx_idx % OS_COM_BUFF_SIZE;

      len++;

      if (pbuff->tx_idx == pbuff->rx_idx)
        break;
    }

  if (len
      && pbuff->status != OS_COM_BUFF_STATUS_BUSY)
    {
      pbuff->status = OS_COM_BUFF_STATUS_BUSY;
    }


  return len;
}

int
os_com_buff_size (os_com_buff_t *pbuff)
{
  if (pbuff == NULL)
    {
      /* Handle is not legal.  */
      return -1;
    }

  if (pbuff->status == OS_COM_BUFF_STATUS_IDLE)
    return 0;

  if (pbuff->tx_idx < pbuff->rx_idx)
    return OS_COM_BUFF_SIZE - pbuff->rx_idx - pbuff->tx_idx;
  else if (pbuff->tx_idx == pbuff->rx_idx)
    return OS_COM_BUFF_SIZE;
  else
    return pbuff->tx_idx - pbuff->rx_idx;
}

int
os_com_buff_clear (os_com_buff_t *pbuff)
{
  if (pbuff == NULL)
    {
      /* Handle is not legal.  */
      return -1;
    }

  pbuff->rx_idx = 0;
  pbuff->tx_idx = 0;
  pbuff->status = OS_COM_BUFF_STATUS_IDLE;

  return 0;
}

