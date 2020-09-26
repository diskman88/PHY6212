#ifndef __IR_H_
#define __IR_H_
#include <stdint.h>
#include <stdbool.h>

#include <yoc_config.h>
#include <aos/log.h>

enum {
    IR_NEC_CODE_D0 = 0,
    IR_NEC_CODE_D1 = 1,
    IR_NEC_CODE_LEAD = 2,
    IR_NEC_CODE_STOP = 3,
    IR_NEC_CODE_C1 = 4,
    IR_NEC_CODE_C2 = 5,
    
    IR_NEC_CODE_NUM,
}ir_nec_code_def_t;

typedef struct {
    uint32_t h;
    uint32_t l;
}ir_nec_code_params_t;

int ir_nec_start_send(uint8_t custom, uint8_t key);

int ir_nec_stop_send();

#endif