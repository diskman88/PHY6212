/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include "mesh_model/mesh_model.h"

static struct bt_mesh_model elem0_root_models[] = {
    MESH_MODEL_CFG_SRV_NULL(),
    MESH_MODEL_CFG_CLI_NULL(),
    MESH_MODEL_HEALTH_SRV_NULL(),
    MESH_MODEL_HEALTH_CLI_NULL(),
    MESH_MODEL_GEN_ONOFF_CLI_NULL(),
    MESH_MODEL_GEN_LEVEL_CLI_NULL(),
    MESH_MODEL_LIGHTNESS_CLI_NULL(),
    MESH_MODEL_LIGHT_CTL_CLI_NULL(),
};

static struct bt_mesh_model elem0_vnd_models[] = {
    MESH_MODEL_VENDOR_CLI_NULL(),
};

static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, elem0_root_models, elem0_vnd_models, 0),
};

/*comp*/
static const struct bt_mesh_comp mesh_comp = {
    .cid = 0x01A8,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

int at_mesh_composition_init()
{
    int ret;
    ret = ble_mesh_model_init(&mesh_comp);
    return ret;
}

