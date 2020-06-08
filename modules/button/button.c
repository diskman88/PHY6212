/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <aos/aos.h>
#include <aos/list.h>
#include <yoc/button.h>

#define MIN_ST_TMOUT (60)
#define MIN_LD_TMOUT (2000)
#define MAX_DD_TMOUT (1000)

#define TAG "button"

typedef enum {
    NONE,
    START,
    CNT,
    HIGH,
    END,
} button_state;

typedef struct button_srv {
    aos_timer_t tmr;
    slist_t head;
} button_srv_t;

typedef struct button_ops {
    int (*read)(button_t *b);
    int (*irq_disable)(button_t *b);
    int (*irq_enable)(button_t *b);
} button_ops_t;

struct button {
    slist_t next;
    long long t_ms;
    long long st_ms;    //enter irq time
    int pin_id;
    gpio_pin_handle_t *pin_hdl;
    int repeat;
    int state;
    int evt_id;
    int old_evt_id;
    int active;
    int irq_flag;
    int evt_flag;
    evt_cb cb;
    void *priv;
    button_ops_t *ops;
    button_param_t param;
};

#define b_param(b) (b->param)

button_srv_t g_button_srv;
#define BUTTON_EVT(button) \
    do {\
        if(button->evt_flag & (1 << button->evt_id)) {\
            button->cb(button->evt_id, button->pin_id, button->priv);\
        }\
    } while(0)

static void irq_disable(button_t *button)
{
    button->ops->irq_disable(button);
}

static void irq_enable(button_t *button)
{
    button->ops->irq_enable(button);
}

void button_irq(button_t *button)
{
    irq_disable(button);
    button->irq_flag = 1;
    button->st_ms = aos_now_ms();
    aos_timer_start(&g_button_srv.tmr);
}

static int reset(button_t *button)
{
    button->state = NONE;
    button->active = 0;

    return 0;
}

static int none_hdl(button_t *button, int level)
{
    if (level == b_param(button).active_level) {
        button->state = START;
        aos_timer_start(&g_button_srv.tmr);
    } else {
        irq_enable(button);
    }

    return 0;
}

static int start_hdl(button_t *button, int level)
{
    if (level == b_param(button).active_level) {
        if ((aos_now_ms() - button->st_ms) >= b_param(button).st_tmout) {
            // button->active = 1;
            button->state = CNT;
        }

        aos_timer_start(&g_button_srv.tmr);
    } else {
        reset(button);
        irq_enable(button);
    }

    return 0;
}

static int cnt_hdl(button_t *button, int level)
{
    if (level == b_param(button).active_level) {
        if ((aos_now_ms() - button->st_ms) >= b_param(button).ld_tmout) {
            button->state = HIGH;
            button->active = 1;
            button->evt_id = BUTTON_PRESS_LONG_DOWN;
        }

        aos_timer_start(&g_button_srv.tmr);
    } else {
        button->state = HIGH;
        button->active = 1;
        button->evt_id = BUTTON_PRESS_DOWN;
        aos_timer_start(&g_button_srv.tmr);
        button->old_evt_id = BUTTON_PRESS_DOWN;
    }

    return 0;
}

static int high_hdl(button_t *button, int level)
{
    if (level == HIGH_LEVEL) {
        button->state = END;
        button->active = 1;

        if (button->evt_id == BUTTON_PRESS_LONG_DOWN) {
            button->evt_id = BUTTON_EVT_END;
        } else {
            button->evt_id = BUTTON_PRESS_UP;
        }

        if (button->old_evt_id == BUTTON_PRESS_DOWN) {
            button->t_ms = aos_now_ms();
        }
        


        irq_enable(button);
    } else {
        aos_timer_start(&g_button_srv.tmr);
    }

    return 0;
}

static int update_repeat(button_t *button)
{
    if (button->state == NONE && button->old_evt_id == BUTTON_PRESS_DOWN \
        && (button->evt_flag & DOUBLE_PRESS_FLAG)) {
        long long past = aos_now_ms() - button->t_ms;

        if (past < b_param(button).dd_tmout) {
            button->repeat = 1;
        } else {
            button->repeat = 0;
        }
    }

    return 0;
}

static int event_call(button_t *button)
{
    if (button->active == 1) {
        if (button->repeat == 0) {
            BUTTON_EVT(button);
            button->active = 0;

            if (button->state == END) {
                reset(button);
            }

        } else {
            if (button->evt_id == BUTTON_PRESS_DOWN || button->evt_id == BUTTON_PRESS_LONG_DOWN) {
                button->evt_id = BUTTON_PRESS_DOUBLE;
                button->old_evt_id = BUTTON_PRESS_DOUBLE;
            }

            button->repeat = 0;
            BUTTON_EVT(button);

            if (button->state == END) {
                reset(button);
            }
        }
    }

    return 0;
}

static int read_level(button_t *button)
{
    return button->ops->read(button);
}

/*
    ----|
*/
static int _button_hdl(button_t *button)
{
    int level;

    update_repeat(button);

    level = read_level(button);

    //LOGD(TAG, "level:%d state:%d", level, button->state);
    aos_timer_stop(&g_button_srv.tmr);

    switch (button->state) {
        case NONE:
            none_hdl(button, level);
            break;

        case START:
            start_hdl(button, level);
            break;

        case CNT:
            cnt_hdl(button, level);
            break;

        case HIGH:
            high_hdl(button, level);
            break;

        default:
            reset(button);
            break;
    }

    //LOGD(TAG, "state:%d", button->state);

    event_call(button);
    return 0;
}

static void button_timer_entry(void *timer, void *arg)
{
    button_t *b;
    int cnt = 0;

    slist_for_each_entry(&g_button_srv.head, b, button_t, next) {
        if (b->irq_flag == 1 || b->state > 0) {
            _button_hdl(b);
            b->irq_flag = 0;
            cnt ++;
        }
    }

    if (cnt == 0) {
        aos_timer_stop(&g_button_srv.tmr);
    }
}

static button_t *button_find(int pin_id)
{
    button_t *b;

    slist_for_each_entry(&g_button_srv.head, b, button_t, next) {
        if (b->pin_id == pin_id) {
            return b;
        }
    }

    return NULL;
}

static int button_new(button_t **button)
{
    *button = aos_zalloc(sizeof(button_t));

    return *button == NULL ? -1 : 0;
}

static int button_add(button_t *button)
{
    slist_add_tail(&button->next, &g_button_srv.head);

    return 0;
}

static int button_set_ops(button_t *button, button_ops_t *ops)
{
    button->ops = ops;

    return 0;
}

static int button_param_init(button_t *button)
{
    button_param_t *p = &button->param;

    p->active_level = LOW_LEVEL;
    p->st_tmout = MIN_ST_TMOUT;
    p->ld_tmout = MIN_LD_TMOUT;
    p->dd_tmout = MAX_DD_TMOUT;

    return 0;
}

static button_t *get_button_hdl(int pin_id)
{
    button_t *b;

    slist_for_each_entry(&g_button_srv.head, b, button_t, next) {
        if (b->pin_id == pin_id) {
            return b;
        }
    }

    return NULL;
}

// csi pin start
static void pin_event(int32_t idx)
{
    button_t *button = NULL;

    button = button_find(idx);

    if (button != NULL) {
        button_irq(button);
    }
}

static gpio_pin_handle_t *csi_gpio_init(int pin_id, button_t *button)
{
    gpio_pin_handle_t *pin_hdl;

    drv_pinmux_config(pin_id, PIN_FUNC_GPIO);

    pin_hdl = csi_gpio_pin_initialize(pin_id, pin_event);
    // csi_gpio_pin_set_evt_priv(pin_hdl, button);
    csi_gpio_pin_config_direction(pin_hdl, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_set_irq(pin_hdl, GPIO_IRQ_MODE_FALLING_EDGE, 0);

    return pin_hdl;
}

static int csi_irq_disable(button_t *button)
{
    csi_gpio_pin_set_irq(button->pin_hdl, GPIO_IRQ_MODE_FALLING_EDGE, 0);

    return 0;
}

static int csi_irq_enable(button_t *button)
{
    csi_gpio_pin_set_irq(button->pin_hdl, GPIO_IRQ_MODE_FALLING_EDGE, 1);

    return 0;
}

static int csi_pin_read(button_t *button)
{
    bool val;

    csi_gpio_pin_read(button->pin_hdl, &val);

    return (val == false) ? LOW_LEVEL : HIGH_LEVEL;
}

static button_ops_t ops = {
    .read = csi_pin_read,
    .irq_disable = csi_irq_disable,
    .irq_enable = csi_irq_enable,
};
// csi pin end

int button_param_cur(int pin_id, button_param_t *p)
{
    button_t *button = get_button_hdl(pin_id);

    if (button) {
        memcpy(p, &button->param, sizeof(button_param_t));
        return 0;
    } else {
        return -1;
    }
}

int button_param_set(int pin_id, button_param_t *p)
{
    button_t *button = get_button_hdl(pin_id);

    if (button) {
        memcpy(&button->param, p, sizeof(button_param_t));
        return 0;
    } else {
        return -1;
    }
}

int button_init(const button_config_t b_tbl[])
{
    button_t *button;
    int ret;

    int i = 0;

    while (1) {
        if (b_tbl[i].evt_flag == 0 && b_tbl[i].cb == NULL) {
            break;
        }

        ret = button_new(&button);

        if (ret == 0) {
            button->pin_id = b_tbl[i].pin_id;
            button->evt_flag = b_tbl[i].evt_flag;
            button->cb = b_tbl[i].cb;
            button->priv = b_tbl[i].priv;
            button_add(button);
            button->pin_hdl = csi_gpio_init(b_tbl[i].pin_id, button);
            button->evt_id = BUTTON_EVT_END;
            button->old_evt_id = BUTTON_EVT_END;
            button_set_ops(button, &ops);
            button_param_init(button);
            csi_gpio_pin_set_irq(button->pin_hdl, GPIO_IRQ_MODE_FALLING_EDGE, 1);

            i ++;
        }
    }

    return i;
}

int button_srv_init(void)
{
    aos_timer_new(&g_button_srv.tmr, button_timer_entry, NULL, 20, 0);
    aos_timer_stop(&g_button_srv.tmr);
    slist_init(&g_button_srv.head);

    return 0;
}
