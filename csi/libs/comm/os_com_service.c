#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <os_port.h>
#include "os_com_service.h"
#include "os_com_port.h"
#include "soc.h"

#define OS_COM_RX_STATE_SD0                0u   /* Wait 'O'.  */
#define OS_COM_RX_STATE_SD1                1u   /* Wait 'S'.  */
#define OS_COM_RX_STATE_SD2                2u   /* Wait 'C'.  */
#define OS_COM_RX_STATE_SD3                3u   /* Wait 'o'.  */
#define OS_COM_RX_STATE_LENH               4u   /* Wait length high byte.  */
#define OS_COM_RX_STATE_LENL               5u   /* Wait length low byte.  */
#define OS_COM_RX_STATE_CTR                6u   /* Wait packat count.  */
#define OS_COM_RX_STATE_PAD                7u   /* wait padding byte.  */
#define OS_COM_RX_STATE_DATA               8u   /* wait datas.  */
#define OS_COM_RX_STATE_CHKSUM             9u   /* wait check sum byte.  */
#define OS_COM_RX_STATE_END                10u  /* wait end byte '/'.  */

#define OS_COM_PKT_SD0                     'O'
#define OS_COM_PKT_SD1                     'S'
#define OS_COM_PKT_SD2                     'C'
#define OS_COM_PKT_SD3                     'o'
#define OS_COM_PKT_END                     '/'

#define OS_COM_PKT_HEAD_TAIL_SIZE          10
#define OS_COM_FORMAT_SIZE                 4

#ifdef   CONFIG_KERNEL_UCOS
#define  OS_TYPE               2
#elif    CONFIG_KERNEL_FREERTOS
#define  OS_TYPE               3
#elif    CONFIG_KERNEL_RHINO
#define  OS_TYPE               4
#endif

typedef int32_t (*cmd_handler_t)(void *pcmd);

typedef struct os_com_cmd
{
  char * name;
  uint16_t rx_format;
  uint16_t tx_format;
  cmd_handler_t handler;
}os_com_cmd_t;

static enum os_com_type_t g_os_com_type;
static uint8_t g_os_com_err = 0;
static uint8_t g_os_com_pkt_ctr;
static void    *g_os_com_sem;

static int32_t rx_query_response (void *pcmd);
static int32_t rx_rd_response (void *pcmd);
static int32_t rx_wr_response (void *pcmd);

os_com_cmd_t g_os_cmd_table[] =
{
    {"Query", OS_COM_FMT_RX_QUERY, OS_COM_FMT_TX_QUERY, rx_query_response},
    {"RD", OS_COM_FMT_RX_RD, OS_COM_FMT_TX_RD, rx_rd_response},
    {"WR", OS_COM_FMT_RX_WR, OS_COM_FMT_TX_WR, rx_wr_response},
    {NULL, 0, 0, NULL},
};

int os_com_service_post_sem (enum os_com_type_t type)
{
  g_os_com_type = type;
  /* Sem Post.  */
  OS_SEM_POST (g_os_com_sem);
  printf ("Post sem\r\n");

  return 0;
}

/***************************************************************************
* Package Format:
*               +-------------------+-------------------+
*               |   'O'   |   'S'   |   'C'   |   'o'   |
*               +-------------------+-------------------+
*               |       Length      | PktCtr  | Padding |
*               +-------------------+-------------------+
*               |                  Data                 |
*               |                   .                   |
*               |                   .                   |
*               |                   .                   |
*               +-------------------+-------------------+
*               | Checksum|   '/'   |
*               +-------------------+
*****************************************************************************/

int
os_com_service_rx_data (enum os_com_type_t type, uint8_t *data, int32_t len)
{
  static int rx_state = OS_COM_RX_STATE_SD0;
  static int dlength = 0;
  static uint8_t chksum = 0;

  while (len--)
    {
      switch (rx_state)
        {
          case OS_COM_RX_STATE_SD0:
            if (*data == (uint8_t)OS_COM_PKT_SD0)
              {
                rx_state = OS_COM_RX_STATE_SD1;
                g_os_com_err = 0;
                dlength = 0;
                chksum = 0;
                /* Clear rx buffer.  */
                os_com_buff_clear (g_com_rx_buff_handle);
                os_com_buff_clear (g_com_tx_buff_handle);
              }
            break;

          case OS_COM_RX_STATE_SD1:
            if (*data == (uint8_t)OS_COM_PKT_SD1)
              rx_state = OS_COM_RX_STATE_SD2;
            else
              rx_state = OS_COM_RX_STATE_SD0;
            break;

          case OS_COM_RX_STATE_SD2:
            if (*data == (uint8_t)OS_COM_PKT_SD2)
              rx_state = OS_COM_RX_STATE_SD3;
            else
              rx_state = OS_COM_RX_STATE_SD0;
            break;

          case OS_COM_RX_STATE_SD3:
            if (*data == (uint8_t)OS_COM_PKT_SD3)
              rx_state = OS_COM_RX_STATE_LENL;
            else
              rx_state = OS_COM_RX_STATE_SD0;

            break;

          case OS_COM_RX_STATE_LENL:
            dlength = *data;
            chksum = *data;
            rx_state = OS_COM_RX_STATE_LENH;
            break;

          case OS_COM_RX_STATE_LENH:
            dlength = dlength| (*data << 8);
            chksum += *data;
            rx_state = OS_COM_RX_STATE_CTR;
            /* check buffer size is enough for this length.  */
            if (dlength >= (OS_COM_BUFF_SIZE - OS_COM_PKT_HEAD_TAIL_SIZE))
              {
                rx_state = OS_COM_RX_STATE_SD0;
                /* Report error to PC.  */
                g_os_com_err = OS_COM_ERR_PKT_SIZE;
              }
            break;

          case OS_COM_RX_STATE_CTR:
            g_os_com_pkt_ctr = *data;
            chksum += *data;
            rx_state = OS_COM_RX_STATE_PAD;
            break;

          case OS_COM_RX_STATE_PAD:
            chksum += *data;
            rx_state = OS_COM_RX_STATE_DATA;
            break;

          case OS_COM_RX_STATE_DATA:
            /* Write data to rx buffer.  */
            if (dlength)
              {
                os_com_buff_write (g_com_rx_buff_handle, data, 1);
                chksum += *data;
                dlength--;
              }
            if (dlength == 0)
              {
                rx_state = OS_COM_RX_STATE_CHKSUM;
              }
            break;

          case OS_COM_RX_STATE_CHKSUM:
            printf ("Get Chksum: %d, %d\r\n", chksum, *data);
            if (chksum == (uint8_t)*data)
              rx_state = OS_COM_RX_STATE_END;
            else
              {
                g_os_com_err = OS_COM_ERR_CHKSUM;
                rx_state = OS_COM_RX_STATE_SD0;
              }
            break;

          case OS_COM_RX_STATE_END:
            if (*data == OS_COM_PKT_END)
              {
                /* Post semaphore.  */
                os_com_service_post_sem  (type);
                rx_state = OS_COM_RX_STATE_SD0;
                return 0;
              }
            else
              {
                g_os_com_err = OS_COM_ERR_END;
                rx_state = OS_COM_RX_STATE_SD0;
              }

          default:
            rx_state = OS_COM_RX_STATE_SD0;
            break;
        }
      /* Reset Communication state.  */
        {
          /* We use the string "ReSt" to reset communication state.  */
          static uint8_t sd0;
          static uint8_t sd1;
          static uint8_t sd2;
          static uint8_t sd3;
          sd0 = sd1;
          sd1 = sd2;
          sd2 = sd3;
          sd3 = *data;
          if (sd0 == 'R' && sd1 == 'e' && sd2 == 'S' && sd3 == 't')
            {
              rx_state = OS_COM_RX_STATE_SD0;
            }
        }

      data++;
    }
  return 1;
}

static int32_t
genarate_response_head (int32_t cnt, int32_t length)
{
  char *pstr = "OSCo";
  os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)pstr, 4);
  os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&length, 2);
  os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&cnt, 1);
  /* The last is padding byte.  */
  cnt = 0;
  os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&cnt, 1);

  return 8;
}

int32_t os_com_gen_response (uint16_t fmt, uint8_t status, uint16_t length, uint8_t *data)
{
  int32_t ret = 0;
  int32_t total = 4;
  uint8_t tmp = 0;
  uint8_t chksum = 0;

  total += length;
  chksum += (total & 0xff) + ((total >> 8) & 0xff);
  ret = genarate_response_head  (g_os_com_pkt_ctr, total);

  chksum += (fmt& 0xff) + ((fmt >> 8) & 0xff);
  ret += os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&fmt, 2);

  chksum += status;
  ret += os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&status, 1);

  tmp = 0;
  ret += os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&tmp, 1);

  OS_ALLOC_CRITICAL_SECTION ();
  OS_ENTER_CRITICAL ();
  ret += os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)data, length);

  while (length--)
    {
      chksum += *data++;
    }
  OS_EXIT_CRITICAL ();

  chksum += g_os_com_pkt_ctr;

  ret += os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&chksum, 1);

  tmp = '/';
  ret += os_com_buff_write (g_com_tx_buff_handle, (uint8_t *)&tmp, 1);

  return ret;
}

static int32_t genarate_error_response (int8_t err)
{
  /* 2bytes format, 1 byte error code, 1byte reserve.  */
  /*    ___________________________
       |  fmt       | err   | rsv  |
        ———————————————————————————
   */
  return os_com_gen_response (OS_COM_FMT_TX_ERR, err, 0, NULL);
}

static int32_t rx_query_response (void *pcmd)
{
  if (os_com_buff_size (g_com_rx_buff_handle) != 2)
    return genarate_error_response (OS_COM_ERR_PKT_SIZE);

  uint16_t query;
  uint16_t length;
  int32_t answer = 0;
  os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&query, 2);

  switch (query)
    {
      /* TODO: fill the followings' response.  */
      case OS_COM_QUERY_MAX_RX_SIZE:
      case OS_COM_QUERY_MAX_TX_SIZE:
        length = 4;
        answer = OS_COM_BUFF_SIZE;
        break;

      case OS_COM_QUERY_VERSION:
        length = 4;
        answer = OS_COM_VERSION;
        break;
      case OS_COM_QUERY_OS:
        length = 4;
        answer = OS_TYPE;
        break;

      case OS_COM_QUERY_FMT_SUPPORT:
      case OS_COM_QUERY_ENDIANNESS_TEST:
      default:
        return genarate_error_response (OS_COM_ERR_QUERY);
    }

  return os_com_gen_response (OS_COM_FMT_TX_QUERY, OS_COM_ERR_NONE, length, (uint8_t *)&answer);
}

static int32_t
rx_rd_response (void *pcmd)
{
  if (os_com_buff_size (g_com_rx_buff_handle) != 6)
    return genarate_error_response (OS_COM_ERR_PKT_SIZE);

  uint32_t addr = 0;
  uint16_t nbytes = 0;

  /* Get nbytes.  */
  os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&nbytes, 2);

  /* Get address.  */
  os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&addr, 4);

  if (nbytes > (OS_COM_BUFF_SIZE - OS_COM_PKT_HEAD_TAIL_SIZE - OS_COM_FORMAT_SIZE))
    return genarate_error_response (OS_COM_ERR_RD_SIZE);

  return os_com_gen_response (OS_COM_FMT_TX_RD, OS_COM_ERR_NONE, nbytes, (uint8_t *)addr);
}

static int32_t
rx_wr_response (void *pcmd)
{
  uint32_t addr = 0;
  uint16_t nbytes = 0;
  uint8_t  data;

  /* Get nbytes.  */
  os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&nbytes, 2);

  /* Get address.  */
  os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&addr, 4);

  while (nbytes--)
    {
      os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&data, 1);
      *(uint8_t *)addr = data;
      addr++;
    }

  return os_com_gen_response (OS_COM_FMT_TX_WR, OS_COM_ERR_NONE, 0, (uint8_t *)NULL);
}

static int32_t os_com_service_parse_pkt (void)
{
  if (os_com_buff_size (g_com_rx_buff_handle) == 0)
    {
      /* RX buff has none data.  */
      return 0;
    }

  /* Get data format.  */
  uint16_t fmt = 0;

  os_com_buff_read (g_com_rx_buff_handle, (uint8_t *)&fmt, 2);

  switch (fmt)
    {
      case OS_COM_FMT_RX_QUERY:
        return rx_query_response (NULL);
      case OS_COM_FMT_RX_RD:
        return rx_rd_response (NULL);
      case OS_COM_FMT_RX_WR:
        return rx_wr_response (NULL);

      case OS_COM_FMT_RX_RD_MULTI:
      case OS_COM_FMT_RX_WR_MULTI:
      default:
        break;
    }
  return 0;
}

static void
os_com_service_task (void *p_arg)
{
  int32_t length = 0;
  os_com_service_tx_func_t pfunc = NULL;
  uint8_t tdata[OS_COM_BUFF_SIZE];

  /* Fix Warning.  */
  (void *)&p_arg;

  printf ("os_com_service_task enter\r\n");

  while (1)
    {
      /* Waiting for semaphore.  */
      printf ("os_com_service_task Hello: before wait sem\r\n");
      OS_SEM_WAIT (g_os_com_sem);
      printf ("os_com_service_task Hello, after wait sem\r\n");

      if (g_os_com_err)
        {
          printf ("Get error packet\r\n");
          length = genarate_error_response (g_os_com_err);
        }
      else
        {
          printf ("os_com_service_task: start parse_pkt\n");
          /* Parse packet.  */
          length = os_com_service_parse_pkt ();
          if (0 == length)
            printf ("os_com_service_task: parse_pkt failed\n");

          /* Start Transmit the reponse.  */
          if (os_com_buff_read (g_com_tx_buff_handle, tdata, length) != length)
            continue;
        }

      pfunc = os_com_get_tx_func (g_os_com_type);
      if (pfunc != NULL)
        pfunc (tdata, length);

    }
}

#define OS_COM_STASK_PTR       NULL
#define OS_COM_STACK_SIZE      1024

void
os_com_service_init (void)
{
  /* OS communication buffer initial.  */
  printf ("os_com_service_init\n");
  os_com_buff_init();

#if OS_COM_RS232
  os_com_rs232_init ();
#endif

  /* Init Semaphore.  */
  OS_SEM_CREATE (g_os_com_sem);
  if (g_os_com_sem == NULL)
    {
      /* TODO: Report an error that Creating semaphore failed.  */
      printf ("sem create failed\r\n");
    }

  /* Create OS communication service.  */
  int32_t err = 0;
  OS_CREATE_TASK (os_com_service_task, "os communication service", OS_COM_STASK_PTR, OS_COM_STACK_SIZE, NULL, OS_COM_SERVICE_PRIO, &err);
  if (!err)
    printf ("Create os_com_service_task Successful\r\n");
}
