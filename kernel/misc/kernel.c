/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <yoc_config.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include <k_err.h>
#include <k_config.h>
#include <k_default_config.h>
#include <k_sys.h>
#include <k_time.h>

#include <k_api.h>
#include <aos/kernel.h>
#include <aos/version.h>
#include <aos/kv.h>
#include <aos/wdt.h>
#include <aos/debug.h>

#include <soc.h>
#include <errno.h>
#include <drv_wdt.h>

#define AOS_MIN_STACK_SIZE       64
#if (RHINO_CONFIG_KOBJ_DYN_ALLOC == 0)
//#warning "RHINO_CONFIG_KOBJ_DYN_ALLOC is disabled!"
#endif

static unsigned int used_bitmap;
static int rhino2stderrno(int ret)
{
    switch (ret) {
        case RHINO_SUCCESS:
        case RHINO_MUTEX_OWNER_NESTED:
            return 0;
        case RHINO_SYS_SP_ERR:
        case RHINO_NULL_PTR:
        case RHINO_MM_FREE_ADDR_ERR:
            return -EFAULT;

        case RHINO_INV_PARAM:
        case RHINO_INV_ALIGN:
        case RHINO_KOBJ_TYPE_ERR:
        case RHINO_MM_POOL_SIZE_ERR:
        case RHINO_MM_ALLOC_SIZE_ERR:
        case RHINO_INV_SCHED_WAY:
        case RHINO_TASK_INV_STACK_SIZE:
        case RHINO_BEYOND_MAX_PRI:
        case RHINO_BUF_QUEUE_INV_SIZE:
        case RHINO_BUF_QUEUE_SIZE_ZERO:
        case RHINO_BUF_QUEUE_MSG_SIZE_OVERFLOW:
        case RHINO_QUEUE_FULL:
        case RHINO_QUEUE_NOT_FULL:
        case RHINO_SEM_OVF:
        case RHINO_WORKQUEUE_EXIST:
        case RHINO_WORKQUEUE_NOT_EXIST:
        case RHINO_WORKQUEUE_WORK_EXIST:
            return -EINVAL;

        case RHINO_KOBJ_BLK:
            return -EAGAIN;

        case RHINO_NO_MEM:
            return -ENOMEM;

        case RHINO_KOBJ_DEL_ERR:
        case RHINO_SCHED_DISABLE:
        case RHINO_SCHED_ALREADY_ENABLED:
        case RHINO_SCHED_LOCK_COUNT_OVF:
        case RHINO_TASK_NOT_SUSPENDED:
        case RHINO_TASK_DEL_NOT_ALLOWED:
        case RHINO_TASK_SUSPEND_NOT_ALLOWED:
        case RHINO_SUSPENDED_COUNT_OVF:
        case RHINO_PRI_CHG_NOT_ALLOWED:
        case RHINO_NOT_CALLED_BY_INTRPT:
        case RHINO_NO_THIS_EVENT_OPT:
        case RHINO_TIMER_STATE_INV:
        case RHINO_BUF_QUEUE_FULL:
        case RHINO_SEM_TASK_WAITING:
        case RHINO_MUTEX_NOT_RELEASED_BY_OWNER:
        case RHINO_WORKQUEUE_WORK_RUNNING:
            return -EPERM;

        case RHINO_TRY_AGAIN:
        case RHINO_WORKQUEUE_BUSY:
            return -EAGAIN;

        default:
            return -1;
    }

    return -1;
}


/* imp in watchdog.c */
//int aos_reboot(void)
int aos_get_hz(void)
{
    return RHINO_CONFIG_TICKS_PER_SECOND;
}

const char *aos_version_get(void)
{
    return aos_get_os_version();
}

#if (RHINO_CONFIG_KOBJ_DYN_ALLOC > 0)
int aos_task_new(const char *name, void (*fn)(void *), void *arg,
                 int stack_size)
{
    int ret;

    ktask_t *task_handle;

    aos_check_return_einval(name && fn && (stack_size >= AOS_MIN_STACK_SIZE));

    ret = (int)krhino_task_dyn_create(&task_handle, name, arg, AOS_DEFAULT_APP_PRI, 0,
                                      stack_size / sizeof(cpu_stack_t), fn, 1u);

    return rhino2stderrno(ret);
}
#endif

int aos_task_new_ext(aos_task_t *task, const char *name, void (*fn)(void *), void *arg,
                     void *stack_buf, int stack_size, int prio)
{
    kstat_t      ret;
    cpu_stack_t *task_stack;

    aos_check_return_einval(task && fn && (stack_size >= AOS_MIN_STACK_SIZE) &&
                         (prio >= 0 && prio < RHINO_CONFIG_PRI_MAX));

    if (stack_buf == NULL) {
#if (RHINO_CONFIG_KOBJ_DYN_ALLOC > 0)
     task_stack = krhino_mm_alloc(stack_size * sizeof(cpu_stack_t) + sizeof(ktask_t), __builtin_return_address(0));
     if (task_stack == NULL) {
         return RHINO_NO_MEM;
     }
#else
        return RHINO_INV_PARAM;
#endif
    } else {
        task_stack = (cpu_stack_t*)stack_buf;
    }

    ret = krhino_task_create(&task->hdl, name, arg, prio, 0, task_stack, stack_size / sizeof(cpu_stack_t), fn, 1);

    if ((ret != RHINO_SUCCESS) && (ret != RHINO_STOPPED)) {
#if (RHINO_CONFIG_KOBJ_DYN_ALLOC > 0)
        krhino_mm_free(task_stack);
#endif
    }

    return rhino2stderrno(ret);
}

void aos_task_wdt_attach(void (*will)(void *), void *args)
{
#ifdef CONFIG_SOFTWDT
    aos_wdt_attach((uint32_t)krhino_cur_task_get(), will, args);
#else
    (void)will;
    (void)args;
#endif
}

void aos_task_wdt_detach()
{
#ifdef CONFIG_SOFTWDT
    uint32_t index = (uint32_t)krhino_cur_task_get();

    aos_wdt_feed(index, 0);
    aos_wdt_detach(index);
#endif
}

void aos_task_wdt_feed(int time)
{
#ifdef CONFIG_SOFTWDT
    ktask_t *task = krhino_cur_task_get();

    if (!aos_wdt_exists((uint32_t)task))
        aos_wdt_attach((uint32_t)task, NULL, (void*)task->task_name);

    aos_wdt_feed((uint32_t)task, time);
#endif
}

void aos_task_exit(int code)
{
    (void)code;
#if (RHINO_CONFIG_KOBJ_DYN_ALLOC > 0)
    krhino_task_dyn_del(NULL);
#endif
}

aos_task_t* aos_task_self()
{
    aos_task_t *task;
    task = (aos_task_t*)krhino_cur_task_get();

    return task;
}

const char *aos_task_name(void)
{
    return krhino_cur_task_get()->task_name;
}

const char *aos_task_get_name(aos_task_t *task)
{
    return task->hdl.task_name;
}

int aos_task_key_create(aos_task_key_t *key)
{
    int i;

    for (i = RHINO_CONFIG_TASK_INFO_NUM - 1; i >= 0; i--) {
        if (!((1 << i) & used_bitmap)) {
            used_bitmap |= 1 << i;
            *key = i;

            return 0;
        }
    }

    return -EINVAL;
}

void aos_task_key_delete(aos_task_key_t key)
{
    if (key >= RHINO_CONFIG_TASK_INFO_NUM) {
        return;
    }

    used_bitmap &= ~(1 << key);
}

#if (RHINO_CONFIG_TASK_INFO > 0)
int aos_task_setspecific(aos_task_key_t key, void *vp)
{
    int ret;
    ret = krhino_task_info_set(krhino_cur_task_get(), key, vp);

    return rhino2stderrno(ret);
}

void *aos_task_getspecific(aos_task_key_t key)
{
    void *vp = NULL;

    krhino_task_info_get(krhino_cur_task_get(), key, &vp);

    return vp;
}
#endif

int aos_mutex_new(aos_mutex_t *mutex)
{
    kstat_t   ret;

    aos_check_return_einval(mutex);

    ret = krhino_mutex_create(&mutex->hdl, "AOS");

    return rhino2stderrno(ret);
}

void aos_mutex_free(aos_mutex_t *mutex)
{
    aos_check_return(mutex);

    krhino_mutex_del(&mutex->hdl);
}

int aos_mutex_lock(aos_mutex_t *mutex, unsigned int timeout)
{
    kstat_t ret;

    aos_check_return_einval(mutex);

    if (timeout == AOS_WAIT_FOREVER) {
        ret = krhino_mutex_lock(&mutex->hdl, RHINO_WAIT_FOREVER);
    } else {
        ret = krhino_mutex_lock(&mutex->hdl, MS2TICK(timeout));
    }

    return rhino2stderrno(ret);
}

int aos_mutex_unlock(aos_mutex_t *mutex)
{
    kstat_t ret;

    aos_check_return_einval(mutex);

    ret = krhino_mutex_unlock(&mutex->hdl);

    return rhino2stderrno(ret);
}

int aos_mutex_is_valid(aos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return 0;
    }

    if (mutex->hdl.blk_obj.obj_type != RHINO_MUTEX_OBJ_TYPE) {
        return 0;
    }

    return 1;
}

#if (RHINO_CONFIG_SEM > 0)
int aos_sem_new(aos_sem_t *sem, int count)
{
    kstat_t ret;

    aos_check_return_einval(sem);

    ret = krhino_sem_create(&sem->hdl, "AOS", count);

    return rhino2stderrno(ret);
}

void aos_sem_free(aos_sem_t *sem)
{
    aos_check_return(sem);

    krhino_sem_del(&sem->hdl);
}

int aos_sem_wait(aos_sem_t *sem, unsigned int timeout)
{
    kstat_t ret;

    aos_check_return_einval(sem);

    if (timeout == AOS_WAIT_FOREVER) {
        ret = krhino_sem_take(&sem->hdl, RHINO_WAIT_FOREVER);
    } else {
        ret = krhino_sem_take(&sem->hdl, MS2TICK(timeout));
    }
    return rhino2stderrno(ret);
}

void aos_sem_signal(aos_sem_t *sem)
{
    aos_check_return(sem);

    krhino_sem_give(&sem->hdl);
}

void aos_sem_signal_all(aos_sem_t *sem)
{
    aos_check_return(sem);

    krhino_sem_give_all(&sem->hdl);
}

int aos_sem_is_valid(aos_sem_t *sem)
{
    if (sem == NULL) {
        return 0;
    }

    if (sem->hdl.blk_obj.obj_type != RHINO_SEM_OBJ_TYPE) {
        return 0;
    }

    return 1;
}

#endif

#if (RHINO_CONFIG_EVENT_FLAG > 0)

int aos_event_new(aos_event_t *event, unsigned int flags)
{
    int ret;

    aos_check_return_einval(event);

    ret = (int)krhino_event_create(&event->hdl, "AOS", flags);

    return rhino2stderrno(ret);
}

void aos_event_free(aos_event_t *event)
{
    aos_check_return(event);

    (void)krhino_event_del(&event->hdl);
}

int aos_event_get
(
    aos_event_t *event,
    unsigned int flags,
    unsigned char opt,
    unsigned int *actl_flags,
    unsigned int timeout
)
{
    kstat_t ret;

    aos_check_return_einval(event);

    if (timeout == AOS_WAIT_FOREVER) {
        ret = krhino_event_get(&event->hdl, (uint32_t)flags, (uint8_t)opt, (uint32_t *)actl_flags, RHINO_WAIT_FOREVER);
    } else {
        ret = krhino_event_get(&event->hdl, (uint32_t)flags, (uint8_t)opt, (uint32_t *)actl_flags, MS2TICK(timeout));
    }

    return rhino2stderrno(ret);
}

int aos_event_set(aos_event_t *event, unsigned int flags, unsigned char opt)
{
    kstat_t ret;

    aos_check_return_einval(event);

    ret = krhino_event_set(&event->hdl, flags, opt);

    return rhino2stderrno(ret);
}

int aos_event_is_valid(aos_event_t *event)
{
    if (event == NULL) {
        return 0;
    }

    if (event->hdl.blk_obj.obj_type != RHINO_EVENT_OBJ_TYPE) {
        return 0;
    }

    return 1;
}
#endif

#if (RHINO_CONFIG_BUF_QUEUE > 0)

int aos_queue_new(aos_queue_t *queue, void *buf, unsigned int size, int max_msg)
{
    kstat_t ret;

    aos_check_return_einval(queue && buf && (size > 0) && (max_msg > 0));

    ret = krhino_buf_queue_create(&queue->hdl, "AOS", buf, size, max_msg);

    return rhino2stderrno(ret);
}

void aos_queue_free(aos_queue_t *queue)
{
    aos_check_return(queue);

    krhino_buf_queue_del(&queue->hdl);
}

int aos_queue_send(aos_queue_t *queue, void *msg, unsigned int size)
{
    int ret;

    aos_check_return_einval(queue && msg && size);

    ret = krhino_buf_queue_send(&queue->hdl, msg, size);

    return rhino2stderrno(ret);
}

int aos_queue_recv(aos_queue_t *queue, unsigned int ms, void *msg,
                   unsigned int *size)
{
    int ret;

    aos_check_return_einval(queue && msg && size);

    if (ms == AOS_WAIT_FOREVER) {
        ret = krhino_buf_queue_recv(&queue->hdl, RHINO_WAIT_FOREVER, msg, size);
    } else {
        ret = krhino_buf_queue_recv(&queue->hdl, MS2TICK(ms), msg, size);
    }

    return rhino2stderrno(ret);
}

int aos_queue_is_valid(aos_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }

    if (queue->hdl.blk_obj.obj_type != RHINO_BUF_QUEUE_OBJ_TYPE) {
        return 0;
    }

    return 1;
}

void *aos_queue_buf_ptr(aos_queue_t *queue)
{
    if (!aos_queue_is_valid(queue)) {
        return NULL;
    }

    return (queue->hdl.buf);
}

int aos_queue_get_count(aos_queue_t *queue)
{
    aos_check_return_einval(queue);

    kbuf_queue_info_t info;
    kstat_t ret = krhino_buf_queue_info_get(&queue->hdl, &info);

    return ret == RHINO_SUCCESS ? info.cur_num : -EPERM;
}
#endif

#if (RHINO_CONFIG_TIMER > 0)
int aos_timer_new(aos_timer_t *timer, void (*fn)(void *, void *),
                  void *arg, int ms, int repeat)
{
    aos_check_return_einval(timer && fn);

    sys_time_t round = repeat == 0 ? 0 : ms;
    kstat_t ret = krhino_timer_create(&timer->hdl, "AOS", (timer_cb_t)fn, MS2TICK(ms), MS2TICK(round), arg, 1);
    return rhino2stderrno(ret);
}

int aos_timer_new_ext(aos_timer_t *timer, void (*fn)(void *, void *),
                      void *arg, int ms, int repeat, unsigned char auto_run)
{
    aos_check_return_einval(timer && fn);

    sys_time_t round = repeat == 0 ? 0 : ms;
    kstat_t ret = krhino_timer_create(&timer->hdl, "AOS", (timer_cb_t)fn, MS2TICK(ms), MS2TICK(round), arg, auto_run);
    return rhino2stderrno(ret);
}

void aos_timer_free(aos_timer_t *timer)
{
    aos_check_return(timer);
}

int aos_timer_start(aos_timer_t *timer)
{
    int ret;

    aos_check_return_einval(timer);

    ret = krhino_timer_start(&timer->hdl);

    return rhino2stderrno(ret);
}

int aos_timer_stop(aos_timer_t *timer)
{
    int ret;

    aos_check_return_einval(timer);

    ret = krhino_timer_stop(&timer->hdl);

    return rhino2stderrno(ret);
}

int aos_timer_change(aos_timer_t *timer, int ms)
{
    int ret;

    aos_check_return_einval(timer);

    ret = krhino_timer_change(&timer->hdl, MS2TICK(ms), MS2TICK(ms));

    return rhino2stderrno(ret);
}

int aos_timer_change_without_repeat(aos_timer_t *timer, int ms)
{
    int ret;

    aos_check_return_einval(timer);

    ret = krhino_timer_change(&timer->hdl, MS2TICK(ms), 0);

    return rhino2stderrno(ret);
}
#endif

#if (RHINO_CONFIG_WORKQUEUE  > 0)
int aos_workqueue_create(aos_workqueue_t *workqueue, int pri, int stack_size)
{
    kstat_t ret;

    aos_check_return_einval(workqueue && stack_size >= AOS_MIN_STACK_SIZE);

    workqueue->hdl = (cpu_stack_t *)aos_malloc(sizeof(kworkqueue_t) + stack_size);

    if (workqueue->hdl == NULL) {
        return -ENOMEM;
    }

    workqueue->stk = (cpu_stack_t*)((char *)workqueue->hdl + sizeof(kworkqueue_t));

    ret = krhino_workqueue_create(workqueue->hdl, "AOS", pri, workqueue->stk,
                                  stack_size / sizeof(cpu_stack_t));

    if (ret != RHINO_SUCCESS) {
        aos_free(workqueue->hdl);
        workqueue->hdl = NULL;
        workqueue->stk = NULL;
    }

    return rhino2stderrno(ret);
}

void aos_workqueue_del(aos_workqueue_t *workqueue)
{
    aos_check_return(workqueue && workqueue->hdl && workqueue->stk);

    krhino_workqueue_del(workqueue->hdl);
    aos_free(workqueue->hdl);
}

int aos_work_init(aos_work_t *work, void (*fn)(void *), void *arg, int dly)
{
    kstat_t  ret;
    //kwork_t *w;

    aos_check_return_einval(work);

    work->hdl = aos_malloc(sizeof(kwork_t));

    if (work->hdl == NULL) {
        return -ENOMEM;
    }

    ret = krhino_work_init(work->hdl, fn, arg, MS2TICK(dly));

    if (ret != RHINO_SUCCESS) {
        aos_free(work->hdl);
        work->hdl = NULL;

    }

    return rhino2stderrno(ret);
}

void aos_work_destroy(aos_work_t *work)
{
    kwork_t *w;

    if (work == NULL) {
        return;
    }

    w = work->hdl;

    if (w->timer != NULL) {
        krhino_timer_stop(w->timer);
        krhino_timer_dyn_del(w->timer);
    }

    aos_free(work->hdl);
    work->hdl = NULL;
}

int aos_work_run(aos_workqueue_t *workqueue, aos_work_t *work)
{
    int ret;

    if ((workqueue == NULL) || (work == NULL)) {
        return -EINVAL;
    }

    ret = krhino_work_run(workqueue->hdl, work->hdl);

    return rhino2stderrno(ret);
}

int aos_work_sched(aos_work_t *work)
{
    int ret;

    if (work == NULL) {
        return -EINVAL;
    }

    ret = krhino_work_sched(work->hdl);

    return rhino2stderrno(ret);
}

int aos_work_cancel(aos_work_t *work)
{
    int ret;

    if (work == NULL) {
        return -EINVAL;
    }

    ret = krhino_work_cancel(work->hdl);

    if (ret != RHINO_SUCCESS) {
        return -EBUSY;
    }

    return 0;
}
#endif

long long aos_now(void)
{
    return krhino_sys_time_get() * 1000 * 1000;
}

long long aos_now_ms(void)
{
    return krhino_sys_time_get();
}

void aos_msleep(int ms)
{
    krhino_task_sleep(MS2TICK(ms));
}

static void trap_c_cb()
{
    aos_except_process(EPERM, NULL, 0, NULL, NULL);
}

extern void (*trap_c_callback)();


void aos_init(void)
{
    krhino_init();

#ifdef CONFIG_KERNEL_PWR_MGMT
    extern void cpu_pwrmgmt_init(void);
    cpu_pwrmgmt_init();
#endif
    // intercept trap_c
    trap_c_callback = trap_c_cb;
}

void aos_start(void)
{
    krhino_start();
}

/* YoC extend aos API */

int aos_get_mminfo(int32_t *total, int32_t *used, int32_t *mfree, int32_t *peak)
{
    aos_check_return_einval(total && used && mfree && peak);
#if (RHINO_CONFIG_MM_TLF > 0)
    *total = g_kmm_head->used_size + g_kmm_head->free_size;
    *used = g_kmm_head->used_size;
    *mfree = g_kmm_head->free_size;
    *peak = g_kmm_head->maxused_size;
#endif
    return 0;
}

int aos_mm_dump(void)
{
#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_MM)
    dumpsys_mm_info_func(NULL, 0);
#endif

    return 0;
}

static int aos_task_list(void *task_array, uint32_t array_items)
{
    if (task_array == NULL || array_items == 0) {
        return 0;
    }

    uint32_t real_tsk_num = 0;
#if (RHINO_CONFIG_SYSTEM_STATS > 0)
    klist_t *taskhead;
    klist_t *taskend;
    klist_t *tmp;
    ktask_t  *task;

    void **tk_tmp = task_array;
    taskhead = &g_kobj_list.task_head;
    taskend  = taskhead;

#ifdef CONFIG_BACKTRACE
    uint32_t task_free;
    size_t irq_flags;
#endif
#ifdef CONFIG_STACK_GUARD
    int      stack_flags;
#endif

    for (tmp = taskhead->next; tmp != taskend; tmp = tmp->next) {
        real_tsk_num ++;
        task  = krhino_list_entry(tmp, ktask_t, task_stats_item);
#ifdef CONFIG_BACKTRACE
        krhino_task_stack_min_free(task, &task_free);
        printf("\n%s:\n\t    state %d, pri %d, stack: total %d, free %d\n",
               task->task_name, task->task_state, task->prio, 4 * task->stack_size, 4 * task_free);
#endif
    }

    if (array_items < real_tsk_num) {
        real_tsk_num = array_items;
    }

    for (tmp = taskhead->next; tmp != taskend && real_tsk_num >= 1 && (array_items--); tmp = tmp->next) {
        task  = krhino_list_entry(tmp, ktask_t, task_stats_item);
        *tk_tmp = task;
        tk_tmp ++;
    }

#ifdef CONFIG_BACKTRACE
    irq_flags = cpu_intrpt_save();

#ifdef CONFIG_STACK_GUARD
    extern int stack_guard_save(void);
    stack_flags = stack_guard_save();
#endif

    for (tmp = taskhead->next; tmp != taskend; tmp = tmp->next) {
        task  = krhino_list_entry(tmp, ktask_t, task_stats_item);
        krhino_task_stack_min_free(task, &task_free);

        printf("\n%s:\n", task->task_name);
        extern int csky_task_backtrace(void *stack, char *buf, int len);
        csky_task_backtrace(task->task_stack, NULL, 0);
    }

#ifdef CONFIG_STACK_GUARD
    extern void stack_guard_restore(int value);
    stack_guard_restore(stack_flags);
#endif

    cpu_intrpt_restore(irq_flags);

#endif /* CONFIG_BACKTRACE */
#endif /* RHINO_CONFIG_SYSTEM_STATS */

    return real_tsk_num;
}

static uint32_t aos_task_get_stack_space(void *task_handle)
{
    if (task_handle == NULL) {
        return 0;
    }

    unsigned int stack_free = 0;
    kstat_t ret = krhino_task_stack_min_free(task_handle, &stack_free);

    if (ret == RHINO_SUCCESS) {
        return 4 * stack_free;
    } else {
        return 0;
    }
}

void aos_task_show_info(void)
{
    int i, task_num;
    #define TASK_MAX_NUM 16
    ktask_t *task_array[sizeof(ktask_t*) * TASK_MAX_NUM];

#if (RHINO_CONFIG_CPU_USAGE_STATS > 0)
    printf("\nCPU USAGE: %d/10000\n", krhino_get_cpu_usage());
#endif

    printf("task       pri status      sp     stack size max used ratio left tick\n");
    printf("---------- --- ------- ---------- ---------- -------- ----- ---------\n");
    task_num = aos_task_list(task_array, TASK_MAX_NUM);

    for (i = 0; i < task_num; i++) {
        printf("%-10s %3d ", task_array[i]->task_name, task_array[i]->prio);

        if (task_array[i]->task_state == K_RDY)            printf("ready  ");
        else if (task_array[i]->task_state == K_PEND)      printf("pend   ");
        else if (task_array[i]->task_state == K_SUSPENDED) printf("suspend");
        else if (task_array[i]->task_state == K_SLEEP)     printf("sleep  ");
        else if (task_array[i]->task_state == K_SEED)      printf("seed   ");
        else if (task_array[i]->task_state == K_DELETED)   printf("deleted");
        else                                               printf("%-7d", task_array[i]->task_state);

        uint32_t strack_size = task_array[i]->stack_size * 4;
        uint32_t min_free = aos_task_get_stack_space(task_array[i]);
        printf(" 0x%08x   %8d %8d   %2d%% %8lld\n",
                (uint32_t)task_array[i]->task_stack_base,
                strack_size,
                strack_size - min_free,
                (strack_size - min_free) * 100 / strack_size,
                task_array[i]->tick_remain);
    }
}

/// Suspend the scheduler.
/// \return time in ticks, for how long the system can sleep or power-down.
void aos_kernel_sched_suspend(void)
{
    if (g_sys_stat == RHINO_RUNNING)
        krhino_sched_disable();
}

/// Resume the scheduler.
/// \param[in]     sleep_ticks   time in ticks for how long the system was in sleep or power-down mode.
void aos_kernel_sched_resume()
{
    if (g_sys_stat == RHINO_RUNNING)
        krhino_sched_enable();
}

void aos_task_yield()
{
    krhino_task_yield();
}

void aos_reboot(void)
{
    extern void drv_reboot(void);
    drv_reboot();
}

#define RHINO_OS_MS_PERIOD_TICK      (1000 / RHINO_CONFIG_TICKS_PER_SECOND)
uint64_t aos_kernel_tick2ms(uint32_t ticks)
{
    return ((uint64_t)ticks * RHINO_OS_MS_PERIOD_TICK);
}

uint64_t aos_kernel_ms2tick(uint32_t ms)
{
    if (ms < RHINO_OS_MS_PERIOD_TICK) {
        return 0;
    }

    return (((uint64_t)ms) / RHINO_OS_MS_PERIOD_TICK);
}

int32_t aos_kernel_suspend(void)
{
    if (is_klist_empty(&g_tick_head))
    {
        return -1;
    }

    ktask_t * p_tcb  = krhino_list_entry(g_tick_head.next, ktask_t, tick_list);
    return  p_tcb->tick_match > g_tick_count ?  p_tcb->tick_match - g_tick_count : 0;
}

void aos_kernel_resume(int32_t ticks)
{
    tick_list_update((tick_i_t)ticks);
    core_sched();
}
