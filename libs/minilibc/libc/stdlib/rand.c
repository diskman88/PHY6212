#include <yoc_config.h>

#include <stdint.h>

#ifdef CONFIG_TEE_CA
#include <drv_tee.h>
int rand(void)
{
    int32_t rval = 0;
    csi_tee_rand_generate((uint8_t *)&rval, 4);

    return rval;
}

void srand(unsigned int seed)
{
    return;
}
#endif
