#ifndef __OS_PORT_H__
#define __OS_PORT_H__

#ifndef NULL
#define NULL 0
#endif

/* The Followinga are OS porting macros.  */

#include <stdlib.h>
#include <csi_config.h>
#include <csi_kernel.h>
//#include <csi_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create Task Macro.  */

#define OS_CREATE_TASK(entry, name, stack, stack_size, args, prio, perr)   \
          do {                                                             \
               void *phandle = NULL;                                       \
               if (csi_kernel_task_new((k_task_entry_t)entry,              \
                      name, 0,                                             \
                      prio,                                                \
                      0, 0, stack_size,                                    \
                      &phandle))                                           \
              printf ("Create task failed\r\n"); \
          } while (0)

#define OS_SEM_CREATE(psem)                                                \
          do {                                                             \
            psem = csi_kernel_sem_new(1, 0);                               \
          } while (0)

#define OS_SEM_WAIT(sem)                                                   \
          do {                                                             \
            csi_kernel_sem_wait(sem, -1);                                  \
          } while (0)

#define OS_SEM_POST(sem)                                                   \
          do {                                                             \
            csi_kernel_sem_post(sem);                                      \
          } while (0)

#define OS_TASK_DELAY(ntick)                                               \
          do {                                                             \
            csi_kernel_delay(ntick);                                       \
          } while (0)

#define OS_ALLOC_CRITICAL_SECTION()        uint32_t cpustatus;
#define OS_ENTER_CRITICAL()                cpustatus = csi_irq_save();
#define OS_EXIT_CRITICAL()                 csi_irq_restore(cpustatus);


#ifdef __cplusplus
}
#endif

#endif
