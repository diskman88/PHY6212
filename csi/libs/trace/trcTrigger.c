#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include <csi_kernel.h>
#include <os_port.h>
#include <trcTrig.h>
#include <trcConfig.h>

#if CONFIG_OS_TRACE_TRIGGER > 0

#define OS_TRIG_STACK_SIZE        512
#define OS_TRIG_PRIO              KPRIO_NORMAL7

static struct trig_cfg g_trig_cfgs[] =
{
    {"Task 1 trig", 1},
};

static uint32_t g_trig_quantity = sizeof (g_trig_cfgs) / sizeof (struct trig_cfg);

struct trig_manager g_trig_manager;

void trace_stop_hook (void);
static void os_trig_manager(void *p_arg);

void trig_init(void)
{
  int i = 0;

  printf ("Trigger Init\n");
  for (;i < g_trig_quantity; i++)
    {
      g_trig_manager.trigs[i].trg_name = &g_trig_cfgs[i].name[0];
      g_trig_manager.trigs[i].id       = g_trig_cfgs[i].id;
      g_trig_manager.trigs[i].armed    = 0;
      g_trig_manager.trigs[i].status   = TRACE_TRIG_STATUS_TRIG_DISARMED;
    }

  g_trig_manager.trig_quantity = g_trig_quantity;
  g_trig_manager.trig_name_length = CONFIG_TRIG_NAME_LEN;
  g_trig_manager.cur_trig = 0;
  g_trig_manager.target_status = TRACE_TRIG_MANAGER_STATUS_READY;
  g_trig_manager.host_status = TRACE_TRIG_MANAGER_STATUS_READY;

  trace_set_stop_hook (trace_stop_hook);

  /* Create trig manager task.  */
  OS_CREATE_TASK (os_trig_manager, "os trigger service", 0, OS_TRIG_STACK_SIZE, NULL, OS_TRIG_PRIO, 0);
}

static int get_index_from_id (uint8_t id)
{
  int idx;
  for (idx = 0; idx < g_trig_quantity; idx++)
    {
      if (g_trig_manager.trigs[idx].id == id)
        return idx;
    }

  return -1;
}

void trig_start (uint8_t id)
{
  int idx = get_index_from_id (id);

  OS_ALLOC_CRITICAL_SECTION();

  if (idx >= 0)
    {
      if (g_trig_manager.trigs[idx].armed
          && g_trig_manager.target_status == TRACE_TRIG_MANAGER_STATUS_READY
          && g_trig_manager.host_status == TRACE_TRIG_MANAGER_STATUS_READY)
        {
          printf ("Trigger Start\r\n");
          OS_ENTER_CRITICAL();
          trace_stop();
          g_trig_manager.target_status = TRACE_TRIG_MANAGER_STATUS_BUSY;
          g_trig_manager.cur_trig = idx;
          g_trig_manager.trigs[idx].status   = TRACE_TRIG_STATUS_TRIG_RECORDING;
          trace_clear();
          trace_start();
          OS_EXIT_CRITICAL();
        }
    }
}

void trace_stop_hook (void)
{
  uint8_t idx = g_trig_manager.cur_trig;

  if (g_trig_manager.target_status == TRACE_TRIG_MANAGER_STATUS_BUSY)
    {
      printf ("Trigger Stop\r\n");
      g_trig_manager.target_status = TRACE_TRIG_MANAGER_STATUS_READY;
      g_trig_manager.host_status = TRACE_TRIG_MANAGER_STATUS_BUSY;
      g_trig_manager.trigs[idx].status   = TRACE_TRIG_STATUS_TRIG_READY;
    }
}

static void
os_trig_manager(void *p_arg)
{
  int idx = 0;

  while (1)
    {
      for (idx = 0; idx < g_trig_quantity; idx++)
        {
          if (g_trig_manager.trigs[idx].status == TRACE_TRIG_STATUS_TRIG_DISARMED
              && g_trig_manager.trigs[idx].armed)
            {
              g_trig_manager.trigs[idx].status = TRACE_TRIG_STATUS_TRIG_ARMED;
              g_trig_manager.host_status = TRACE_TRIG_MANAGER_STATUS_READY;
              printf ("trig manager status: TRACE_TRIG_STATUS_TRIG_ARMED\r\n");
            }
          else if (g_trig_manager.trigs[idx].status == TRACE_TRIG_STATUS_TRIG_ARMED
                   && !g_trig_manager.trigs[idx].armed)
            {
              g_trig_manager.trigs[idx].status = TRACE_TRIG_STATUS_TRIG_DISARMED;
              trace_stop();
              printf ("trig manager status: TRACE_TRIG_STATUS_TRIG_DISARMED 1\r\n");
            }
          else if (g_trig_manager.trigs[idx].status == TRACE_TRIG_STATUS_TRIG_READY
                   && !g_trig_manager.trigs[idx].armed)
            {
              g_trig_manager.trigs[idx].status = TRACE_TRIG_STATUS_TRIG_DISARMED;
              trace_stop();
              printf ("trig manager status: TRACE_TRIG_STATUS_TRIG_DISARMED 1\r\n");
            }
          else if (g_trig_manager.trigs[idx].status == TRACE_TRIG_STATUS_TRIG_IGNORED
                   && !g_trig_manager.trigs[idx].armed)
            {
              g_trig_manager.trigs[idx].status = TRACE_TRIG_STATUS_TRIG_DISARMED;
              trace_stop();
              printf ("trig manager status: TRACE_TRIG_STATUS_TRIG_DISARMED 3\r\n");
            }
        }

      OS_TASK_DELAY(50);
    }
}


#endif /* CONFIG_OS_TRACE_TRIGGER */


