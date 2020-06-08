#include <yoc/atserver.h>
#include <yoc/at_cmd.h>

#ifdef CONFIG_AT
/**< test Called when recv cmd is AT+<x>=? TEST_CMD*/
/**< query Called when recv cmd is AT+<x>? READ_CMD*/
/**< setup Called when recv cmd is AT+<x>=<...> WRITE_CMD*/
/**< exec Called when recv cmd is AT+<x> EXECUTE_CMD*/

const atserver_cmd_t at_cmd[] = {
    AT,
    AT_HELP,
    AT_FWVER,
    AT_BT_BAUD,
    AT_BT_REBOOT,
    AT_BT_RST,
//for ble
    AT_BT_MAC,
    AT_BT_SLEEP,
    AT_BT_TXPOWER,
    AT_BT_NAME,
    AT_BT_ROLE,
    AT_BT_CONN_UPDATE,
    AT_BT_CONN_INFO,
    AT_BT_DISCONN,
    AT_BT_TX,
    AT_BT_AUTO_ADV,
    AT_BT_ADV,
    AT_BT_AUTO_CONN,
    AT_BT_FIND,
    AT_BT_CONN,
#ifdef CONFIG_AT_OTA
    AT_OTASTART,
    AT_OTASTOP,
    AT_OTAFINISH,
    AT_OTAGETINFO,
    AT_OTAPOST,
#endif
    AT_NULL,
};
#endif
