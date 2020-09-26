#ifndef __TRCTRIG_H__
#define __TRCTRIG_H__

#include <stdint.h>
#include <trcUser.h>
#include <trcConfig.h>

#ifndef CONFIG_TRACE_MAX_TRIGS
#define CONFIG_TRACE_MAX_TRIGS 3
#endif

#ifndef CONFIG_TRIG_NAME_LEN
#define CONFIG_TRIG_NAME_LEN   25
#endif

enum trig_status
{
  TRACE_TRIG_STATUS_TRIG_DISARMED       = 0u,
  TRACE_TRIG_STATUS_TRIG_ARMED          = 1u,
  TRACE_TRIG_STATUS_TRIG_RECORDING      = 2u,
  TRACE_TRIG_STATUS_TRIG_READY          = 3u,
  TRACE_TRIG_STATUS_TRIG_IGNORED        = 4u
};

enum trig_manager_status
{
  TRACE_TRIG_MANAGER_STATUS_IDLE        = 0u,
  TRACE_TRIG_MANAGER_STATUS_READY       = 1u,
  TRACE_TRIG_MANAGER_STATUS_BUSY        = 2u,
};


struct trig
{
  char *trg_name;            /* Trigger Name.  */
  enum trig_status status;   /* Trigger Status.  */
  uint8_t id;                /* Trigger ID.  */
  uint8_t armed;             /* Flag set by PC to enable/disable trigger.  */
  uint16_t reserve;
};

struct trig_manager
{
  struct trig trigs[CONFIG_TRACE_MAX_TRIGS];   /* All triggers infomations.  */
  uint32_t trig_name_length;                   /* Name length per trigger.  */
  enum trig_manager_status target_status;      /* Trigger manager status.  */
  enum trig_manager_status host_status;        /* Trigger manager status.  */
  uint8_t trig_quantity;                       /* Trigger quantity.  */
  uint8_t cur_trig;                            /* Current trigger index.  */
};

struct trig_cfg
{
  char name[CONFIG_TRIG_NAME_LEN];
  uint8_t id;
};

#if CONFIG_OS_TRACE_TRIGGER > 0
void trig_init(void);
#else
#define trig_init()
#endif

#endif /* __TRCTRIG_H__ */
