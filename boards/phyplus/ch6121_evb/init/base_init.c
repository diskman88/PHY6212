#include <yoc_config.h>

#include <aos/version.h>
#include <aos/log.h>
#include <aos/kv.h>
#include <yoc/yoc.h>
#include <devices/devicelist.h>

void board_base_init(void)
{
    uart_csky_register(0);

    spiflash_csky_register(0);
}
