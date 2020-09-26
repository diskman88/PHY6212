/*
 * Copyright (C) 2017-2019 C-SKY Microsystems Co., Ltd. All rights reserved.
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

/******************************************************************************
 * @file     dw_wdt.c
 * @brief    CSI Source File for WDT Driver
 * @version  V1.0
 * @date     24. May 2019
 ******************************************************************************/

#include <csi_config.h>
#include <stdio.h>
#include <drv_irq.h>
#include <drv_wdt.h>
#include <dw_wdt.h>
#include <soc.h>
#include <clock.h>

#define ERR_WDT(errno) (CSI_DRV_ERRNO_WDT_BASE | errno)
#define WDT_NULL_PARAM_CHK(para)  HANDLE_PARAM_CHK(para, ERR_WDT(DRV_ERROR_PARAMETER))
#define SYSTEM_CLOCK_MS (30000 / 1000)

uint32_t timeout_ms[16];

typedef struct {
#ifdef CONFIG_LPM
    uint8_t wdt_power_status;
    uint32_t wdt_regs_saved[2];
#endif
    uint32_t base;
    uint32_t irq;
    wdt_event_cb_t cb_event;
} dw_wdt_priv_t;

extern int32_t target_get_wdt_count(void);
extern int32_t target_get_wdt(int32_t idx, uint32_t *base, uint32_t *irq, void **handler);

static dw_wdt_priv_t wdt_instance[CONFIG_WDT_NUM];

static inline void dw_wdt_enable(dw_wdt_reg_t *addr)
{
    addr->WDT_CR = 0x1F;
}

static inline void dw_wdt_disable(dw_wdt_reg_t *addr)
{
    addr->WDT_CR = 0;
}


void dw_wdt_irqhandler(int32_t idx)
{
    dw_wdt_priv_t *wdt_priv = &wdt_instance[idx];
    dw_wdt_reg_t *addr = (dw_wdt_reg_t *)(wdt_priv->base);

    addr->WDT_EOI;

    if (wdt_priv->cb_event) {
        wdt_priv->cb_event(idx, WDT_EVENT_TIMEOUT);
    }
}

#ifdef CONFIG_LPM
static void manage_clock(wdt_handle_t handle, uint8_t enable)
{
    drv_clock_manager_config(CLOCK_MANAGER_WDT, enable);
}

static void do_prepare_sleep_action(wdt_handle_t handle)
{
    dw_wdt_priv_t *wdt_priv = handle;
    uint32_t *wbase = (uint32_t *)(wdt_priv->base);
    registers_save(wdt_priv->wdt_regs_saved, wbase, 2);
}

static void do_wakeup_sleep_action(wdt_handle_t handle)
{
    dw_wdt_priv_t *wdt_priv = handle;
    uint32_t *wbase = (uint32_t *)(wdt_priv->base);
    registers_restore(wbase, wdt_priv->wdt_regs_saved, 2);
}
#endif

/**
  \brief       Initialize WDT Interface. 1. Initializes the resources needed for the WDT interface 2.registers event callback function
  \param[in]   idx   wdt index
  \param[in]   cb_event  Pointer to \ref wdt_event_cb_t
  \return      pointer to wdt instance
*/
wdt_handle_t csi_wdt_initialize(int32_t idx, wdt_event_cb_t cb_event)
{
    if (idx < 0 || idx >= CONFIG_WDT_NUM) {
        return NULL;
    }

    uint32_t base = 0u;
    uint32_t irq = 0u;
    void *handler;

    clk_gate_enable(MOD_WDT);

    int32_t real_idx = target_get_wdt(idx, &base, &irq, &handler);

    if (real_idx != idx) {
        return NULL;
    }

    dw_wdt_priv_t *wdt_priv = &wdt_instance[idx];
    wdt_priv->base = base;
    wdt_priv->irq  = irq;

    wdt_priv->cb_event = cb_event;

    uint8_t i;

    for (i = 0; i < sizeof(timeout_ms) / sizeof(uint32_t); i++) {
        timeout_ms[i] = (uint32_t)(0x10000 << i) / SYSTEM_CLOCK_MS;
    }

#ifdef CONFIG_LPM
    csi_wdt_power_control(wdt_priv, DRV_POWER_FULL);
#endif

    drv_irq_register(wdt_priv->irq, handler);
    drv_irq_enable(wdt_priv->irq);

    return (wdt_handle_t)wdt_priv;
}

/**
  \brief       De-initialize WDT Interface. stops operation and releases the software resources used by the interface
  \param[in]   instance  wdt instance to operate.
  \return      \ref execution_status
*/
int32_t csi_wdt_uninitialize(wdt_handle_t handle)
{
    WDT_NULL_PARAM_CHK(handle);

    dw_wdt_priv_t *wdt_priv = handle;

    wdt_priv->cb_event = NULL;
    drv_irq_disable(wdt_priv->irq);
    drv_irq_unregister(wdt_priv->irq);
    clk_gate_disable(MOD_WDT);

#ifdef CONFIG_LPM
    csi_wdt_power_control(wdt_priv, DRV_POWER_OFF);
#endif
    return 0;
}

int32_t csi_wdt_power_control(wdt_handle_t handle, csi_power_stat_e state)
{
    WDT_NULL_PARAM_CHK(handle);

    if (state == DRV_POWER_FULL) {
        clk_gate_enable(MOD_WDT);
        return 0;
    } else if (state == DRV_POWER_OFF) {
        clk_gate_disable(MOD_WDT);
        return 0;
    } else {
#ifdef CONFIG_LPM
        power_cb_t callback = {
            .wakeup = do_wakeup_sleep_action,
            .sleep = do_prepare_sleep_action,
            .manage_clock = manage_clock
        };
        return drv_soc_power_control(handle, state, &callback);
#else
        return ERR_WDT(DRV_ERROR_UNSUPPORTED);
#endif
    }
}

/**
  \brief       Set the WDT value. value = (2^t*0xffff * 10^6 /freq)/10^3(t: 0 ~ 15).
  \param[in]   handle wdt handle to operate.
  \param[in]   value     the timeout value(ms) \ref:timeout_ms[]
  \return      \ref execution_status
*/
int32_t csi_wdt_set_timeout(wdt_handle_t handle, uint32_t value)
{
    WDT_NULL_PARAM_CHK(handle);
    uint32_t i = 0u;

    for (i = 0; i <= 15 ; i++) {
        if (timeout_ms[i] >= value) {
            break;
        }

        if (i == 15) {
            return ERR_WDT(DRV_ERROR_PARAMETER);
        }
    }

    dw_wdt_priv_t *wdt_priv = handle;
    dw_wdt_reg_t *addr = (dw_wdt_reg_t *)(wdt_priv->base);

    uint32_t config = addr->WDT_CR;
    uint32_t en_stat = 0;   /*origin wdt enable status*/

    if ((config & 0x1) != 0) {
        en_stat = 1;
    }

    config = 0;
    addr->WDT_CR = config;

    AP_PCR->RESET2 |= 0x4;

    /*before configuration, must disable wdt first*/
    dw_wdt_disable(addr);
    i += i << 4;
    addr->WDT_TORR = i;

    if (en_stat == 1) {
        dw_wdt_enable(addr);
        csi_wdt_restart(handle);
    }

    return 0;
}

/**
  \brief       Start the WDT.
  \param[in]   handle wdt handle to operate.
  \return      \ref execution_status
*/
int32_t csi_wdt_start(wdt_handle_t handle)
{
    WDT_NULL_PARAM_CHK(handle);

    dw_wdt_priv_t *wdt_priv = handle;
    dw_wdt_reg_t *addr = (dw_wdt_reg_t *)(wdt_priv->base);

    dw_wdt_enable(addr);
    csi_wdt_restart(handle);
    return 0;
}

/**
  \brief       Stop the WDT.
  \param[in]   handle wdt handle to operate.
  \return      \ref execution_status
*/
int32_t csi_wdt_stop(wdt_handle_t handle)
{
    WDT_NULL_PARAM_CHK(handle);

    return ERR_WDT(DRV_ERROR_UNSUPPORTED);
}

/**
  \brief       Restart the WDT.
  \param[in]   handle wdt handle to operate.
  \return      \ref execution_status
*/
int32_t csi_wdt_restart(wdt_handle_t handle)
{
    WDT_NULL_PARAM_CHK(handle);

    dw_wdt_priv_t *wdt_priv = handle;
    dw_wdt_reg_t *addr = (dw_wdt_reg_t *)(wdt_priv->base);

    addr->WDT_CRR = DW_WDT_CRR_RESET;

    return 0;
}

/**
  \brief       Read the WDT Current value.
  \param[in]   handle wdt handle to operate.
  \param[in]   value     Pointer to the Value.
  \return      \ref execution_status
*/
int32_t csi_wdt_read_current_value(wdt_handle_t handle, uint32_t *value)
{
    WDT_NULL_PARAM_CHK(handle);
    WDT_NULL_PARAM_CHK(value);

    dw_wdt_priv_t *wdt_priv = handle;
    dw_wdt_reg_t *addr = (dw_wdt_reg_t *)(wdt_priv->base);

    *value = addr->WDT_CCVR;
    return 0;
}

