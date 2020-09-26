#include "trcKernelPort.h"

#if TRACE_CFG_EN > 0
#define  TRACE_INIT()                                      trace_init_data()
#define  TRACE_START()                                     trace_start()
#define  TRACE_STOP()                                      trace_stop()
#define  TRACE_CLEAR()                                     trace_clear()
#else
#define  TRACE_INIT()
#define  TRACE_START()
#define  TRACE_STOP()
#define  TRACE_CLEAR()
#endif


#if TRACE_CFG_EN > 0
#define  TRACE_USR_EVT_CREATE(evt_name)                    trace_open_label(evt_name)
#define  TRACE_USR_EVT_LOG(hnd)                            trace_user_event(hnd)
#define  TRACE_PRINTF(hnd, format_str, ...)                trace_printf(hnd, format_str, __VA_ARGS__)
#else
#define  TRACE_USR_EVT_CREATE(evt_name)
#define  TRACE_USR_EVT_LOG(hnd)
#define  TRACE_PRINTF(hnd, format_str, ...)
#endif


#if TRACE_CFG_EN > 0

#define  TRACE_OS_TICK_INCREMENT(OSTickCtr)                 traceTASK_INCREMENT_TICK(OSTickCtr)
#define  TRACE_OS_TASK_CREATE(p_tcb)                        traceTASK_CREATE(p_tcb)
#define  TRACE_OS_TASK_CREATE_FAILED(p_tcb)                 traceTASK_CREATE_FAILED(p_tcb)
#define  TRACE_OS_TASK_DEL(p_tcb)                           traceTASK_DELETE(p_tcb)
#define  TRACE_OS_TASK_READY(p_tcb)                         traceMOVED_TASK_TO_READY_STATE(p_tcb)
#define  TRACE_OS_TASK_SWITCHED_IN(p_tcb)                   traceTASK_SWITCHED_IN(p_tcb)
#define  TRACE_OS_TASK_DLY(dly_ticks)                       traceTASK_DELAY(dly_ticks)
#define  TRACE_OS_TASK_SUSPEND(p_tcb)                       traceTASK_SUSPEND(p_tcb)
#define  TRACE_OS_TASK_RESUME(p_tcb)                        traceTASK_RESUME(p_tcb)

#define  TRACE_OS_ISR_REGISTER(isr_id, isr_name, isr_prio)  traceISR_REGISTER(isr_id, isr_name, isr_prio)
#define  TRACE_OS_ISR_BEGIN(isr_id)                         traceISR_BEGIN(isr_id)
#define  TRACE_OS_ISR_END()                                 traceISR_END()

#define  TRACE_OS_TASK_MSG_Q_CREATE(p_msg_q, p_name)        traceTASK_MSG_Q_CREATE(TASK_MSG_Q, p_msg_q, "msg_q")
#define  TRACE_OS_TASK_MSG_Q_POST(p_msg_q)                  traceTASK_MSG_Q_SEND(TASK_MSG_Q, p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_POST_FAILED(p_msg_q)           traceTASK_MSG_Q_SEND_FAILED(TASK_MSG_Q, p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_PEND(p_msg_q)                  traceTASK_MSG_Q_RECEIVE(TASK_MSG_Q, p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_PEND_FAILED(p_msg_q)           traceTASK_MSG_Q_RECEIVE_FAILED(TASK_MSG_Q, p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_PEND_BLOCK(p_msg_q)            traceTASK_MSG_Q_RECEIVE_BLOCK(TASK_MSG_Q, p_msg_q)

#define  TRACE_OS_TASK_SEM_CREATE(p_tcb, p_name)            traceTASK_SEM_CREATE(TASK_SEM, p_tcb, "task_sem")
#define  TRACE_OS_TASK_SEM_POST(p_tcb)                      traceTASK_SEM_SEND(TASK_SEM, p_tcb)
#define  TRACE_OS_TASK_SEM_POST_FAILED(p_tcb)               traceTASK_SEM_SEND_FAILED(TASK_SEM, p_tcb)
#define  TRACE_OS_TASK_SEM_PEND(p_tcb)                      traceTASK_SEM_RECEIVE(TASK_SEM, p_tcb)
#define  TRACE_OS_TASK_SEM_PEND_FAILED(p_tcb)               traceTASK_SEM_RECEIVE_FAILED(TASK_SEM, p_tcb)
#define  TRACE_OS_TASK_SEM_PEND_BLOCK(p_tcb)                traceTASK_SEM_RECEIVE_BLOCK(TASK_SEM, p_tcb)

#define  TRACE_OS_MUTEX_CREATE(p_mutex, p_name)             traceMUTEX_CREATE(MUTEX, p_mutex, "Mutex")
#define  TRACE_OS_MUTEX_DEL(p_mutex)                        traceMUTEX_DELETE(MUTEX, p_mutex)
#define  TRACE_OS_MUTEX_POST(p_mutex)                       traceMUTEX_SEND(MUTEX, p_mutex)
#define  TRACE_OS_MUTEX_POST_FAILED(p_mutex)                traceMUTEX_SEND_FAILED(MUTEX, p_mutex)
#define  TRACE_OS_MUTEX_PEND(p_mutex)                       traceMUTEX_RECEIVE(MUTEX, p_mutex)
#define  TRACE_OS_MUTEX_PEND_FAILED(p_mutex)                traceMUTEX_RECEIVE_FAILED(MUTEX, p_mutex)
#define  TRACE_OS_MUTEX_PEND_BLOCK(p_mutex)                 traceMUTEX_RECEIVE_BLOCK(MUTEX, p_mutex)

#define  TRACE_OS_MUTEX_TASK_PRIO_INHERIT(p_tcb, prio)      traceTASK_PRIORITY_INHERIT(p_tcb, prio)
#define  TRACE_OS_MUTEX_TASK_PRIO_DISINHERIT(p_tcb, prio)   traceTASK_PRIORITY_DISINHERIT(p_tcb, prio)

#define  TRACE_OS_SEM_CREATE(p_sem, p_name)                 traceSEM_CREATE(SEM, p_sem, "sem")
#define  TRACE_OS_SEM_DEL(p_sem)                            traceSEM_DELETE(SEM, p_sem)
#define  TRACE_OS_SEM_POST(p_sem)                           traceSEM_SEND(SEM, p_sem)
#define  TRACE_OS_SEM_POST_FAILED(p_sem)                    traceSEM_SEND_FAILED(SEM, p_sem)
#define  TRACE_OS_SEM_PEND(p_sem)                           traceSEM_RECEIVE(SEM, p_sem)
#define  TRACE_OS_SEM_PEND_FAILED(p_sem)                    traceSEM_RECEIVE_FAILED(SEM, p_sem)
#define  TRACE_OS_SEM_PEND_BLOCK(p_sem)                     traceSEM_RECEIVE_BLOCK(SEM, p_sem)

#define  TRACE_OS_Q_CREATE(p_q, p_name)                     traceQ_CREATE(Q, p_q, "q")
#define  TRACE_OS_Q_DEL(p_q)                                traceQ_DELETE(Q, p_q)
#define  TRACE_OS_Q_POST(p_q)                               traceQ_SEND(Q, p_q)
#define  TRACE_OS_Q_POST_FAILED(p_q)                        traceQ_SEND_FAILED(Q, p_q)
#define  TRACE_OS_Q_PEND(p_q)                               traceQ_RECEIVE(Q, p_q)
#define  TRACE_OS_Q_PEND_FAILED(p_q)                        traceQ_RECEIVE_FAILED(Q, p_q)
#define  TRACE_OS_Q_PEND_BLOCK(p_q)                         traceQ_RECEIVE_BLOCK(Q, p_q)

#define  TRACE_OS_MEM_CREATE(p_mem, p_name)                 traceMEM_CREATE(MEM, p_mem, "mem")
#define  TRACE_OS_MEM_PUT(p_mem)                            traceMEM_SEND(MEM, p_mem)
#define  TRACE_OS_MEM_PUT_FAILED(p_mem)                     traceMEM_SEND_FAILED(MEM, p_mem)
#define  TRACE_OS_MEM_GET(p_mem)                            traceMEM_RECEIVE(MEM, p_mem)
#define  TRACE_OS_MEM_GET_FAILED(p_mem)                     traceMEM_RECEIVE_FAILED(MEM, p_mem)

#define  TRACE_OS_FLAG_CREATE(p_grp, p_name)
#define  TRACE_OS_FLAG_DEL(p_grp)
#define  TRACE_OS_FLAG_POST(p_grp)
#define  TRACE_OS_FLAG_POST_FAILED(p_grp)
#define  TRACE_OS_FLAG_PEND(p_grp)
#define  TRACE_OS_FLAG_PEND_FAILED(p_grp)
#define  TRACE_OS_FLAG_PEND_BLOCK(p_grp)

#else

#define  TRACE_OS_TICK_INCREMENT(OSTickCtr)

#define  TRACE_OS_TASK_CREATE(p_tcb)
#define  TRACE_OS_TASK_CREATE_FAILED(p_tcb)
#define  TRACE_OS_TASK_DEL(p_tcb)
#define  TRACE_OS_TASK_READY(p_tcb)
#define  TRACE_OS_TASK_SWITCHED_IN(p_tcb)
#define  TRACE_OS_TASK_DLY(dly_ticks)
#define  TRACE_OS_TASK_SUSPEND(p_tcb)
#define  TRACE_OS_TASK_RESUME(p_tcb)


#define  TRACE_OS_ISR_REGISTER(isr_id, isr_name, isr_prio)
#define  TRACE_OS_ISR_BEGIN(isr_id)
#define  TRACE_OS_ISR_END()

#define  TRACE_OS_TASK_MSG_Q_CREATE(p_msg_q, p_name)
#define  TRACE_OS_TASK_MSG_Q_POST(p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_POST_FAILED(p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_PEND(p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_PEND_FAILED(p_msg_q)
#define  TRACE_OS_TASK_MSG_Q_PEND_BLOCK(p_msg_q)

#define  TRACE_OS_TASK_SEM_CREATE(p_tcb, p_name)
#define  TRACE_OS_TASK_SEM_POST(p_tcb)
#define  TRACE_OS_TASK_SEM_POST_FAILED(p_tcb)
#define  TRACE_OS_TASK_SEM_PEND(p_tcb)
#define  TRACE_OS_TASK_SEM_PEND_FAILED(p_tcb)
#define  TRACE_OS_TASK_SEM_PEND_BLOCK(p_tcb)
#define  TRACE_OS_MUTEX_CREATE(p_mutex, p_name)
#define  TRACE_OS_MUTEX_DEL(p_mutex)
#define  TRACE_OS_MUTEX_POST(p_mutex)
#define  TRACE_OS_MUTEX_POST_FAILED(p_mutex)
#define  TRACE_OS_MUTEX_PEND(p_mutex)
#define  TRACE_OS_MUTEX_PEND_FAILED(p_mutex)
#define  TRACE_OS_MUTEX_PEND_BLOCK(p_mutex)

#define  TRACE_OS_MUTEX_TASK_PRIO_INHERIT(p_tcb, prio)
#define  TRACE_OS_MUTEX_TASK_PRIO_DISINHERIT(p_tcb, prio)

#define  TRACE_OS_SEM_CREATE(p_sem, p_name)
#define  TRACE_OS_SEM_DEL(p_sem)
#define  TRACE_OS_SEM_POST(p_sem)
#define  TRACE_OS_SEM_POST_FAILED(p_sem)
#define  TRACE_OS_SEM_PEND(p_sem)
#define  TRACE_OS_SEM_PEND_FAILED(p_sem)
#define  TRACE_OS_SEM_PEND_BLOCK(p_sem)

#define  TRACE_OS_Q_CREATE(p_q, p_name)
#define  TRACE_OS_Q_DEL(p_q)
#define  TRACE_OS_Q_POST(p_q)
#define  TRACE_OS_Q_POST_FAILED(p_q)
#define  TRACE_OS_Q_PEND(p_q)
#define  TRACE_OS_Q_PEND_FAILED(p_q)
#define  TRACE_OS_Q_PEND_BLOCK(p_q)

#define  TRACE_OS_FLAG_CREATE(p_grp, p_name)
#define  TRACE_OS_FLAG_DEL(p_grp)
#define  TRACE_OS_FLAG_POST(p_grp)
#define  TRACE_OS_FLAG_POST_FAILED(p_grp)
#define  TRACE_OS_FLAG_PEND(p_grp)
#define  TRACE_OS_FLAG_PEND_FAILED(p_grp)
#define  TRACE_OS_FLAG_PEND_BLOCK(p_grp)

#define  TRACE_OS_MEM_CREATE(p_mem, p_name)
#define  TRACE_OS_MEM_PUT(p_mem)
#define  TRACE_OS_MEM_PUT_FAILED(p_mem)
#define  TRACE_OS_MEM_GET(p_mem)
#define  TRACE_OS_MEM_GET_FAILED(p_mem)

#endif


