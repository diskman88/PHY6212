/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
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
#include "log.h"
#include <aos/aos.h>
#include <aos/kernel.h>
#include "common.h"
#include "dtm_test.h"
#include "dut_uart_driver.h"

 //DTM STATE
#define RF_PHY_DTM_IDL                              0
#define RF_PHY_DTM_CMD                              1
#define RF_PHY_DTM_EVT                              2
#define RF_PHY_DTM_TEST                             3

/*******************************************************************************
 * Global Var
 */
static int dtmState = 0;

/*Caution: Here we need recv 2 bytes in one packet, so UART FIFO must be 2 more */
void dtm_test_loop(char dut_data[], uint32_t len)
{
    while (1) {
        if (dtmState == RF_PHY_DTM_IDL) {
            if (len >= 2) {
                rf_phy_set_dtm_cmd(dut_data);
                dtmState = RF_PHY_DTM_CMD;
            } else {
                break;
            }
            //=================== cmd parsing  =====================
        } else if (dtmState == RF_PHY_DTM_CMD) {
            /* parse dtm command and prepare the test */
            rf_phy_dtm_cmd_parse();
            dtmState = RF_PHY_DTM_EVT;
            //=================== send event   =====================
        } else if (dtmState == RF_PHY_DTM_EVT) {
            /* check the event and response at first */
            uint8_t ret = rf_phy_dtm_test_evt_send();
            if (ret != 0) {
                /* Fail to send event */
                break;
            }
            dtmState = RF_PHY_DTM_TEST;
            //=================== TEST Start    =====================
        } else if (dtmState == RF_PHY_DTM_TEST) {
            rf_phy_dtm_trigged();
            rf_phy_dtm_reset_pkg_count();

            /* when new cmd arrived, exit dtm test loop */
            while (g_dut_queue_num == 0) {
                //TX BURST re-trigger
                rf_phy_dtm_test_tx_mod_burst();
            }
            break;
        }
    }

    dtmState = RF_PHY_DTM_IDL;
    return;
}

