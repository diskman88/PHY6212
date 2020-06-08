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
    AT_CGMR,
    AT_FWVER,
    AT_SYSTIME,
    AT_SAVE,
    AT_FACTORYW,
    AT_FACTORYR,
    AT_REBOOT,
    AT_EVENT,
    AT_ECHO,
#ifdef CONFIG_YOC_LPM
    AT_SLEEP,
#endif
#if !defined(CONFIG_PRODUCT_MODEL)
    AT_MODEL,
#endif

//#ifdef CONFIG_FOTA
#if 0
    AT_FOTASTART,
    AT_FOTAGETFULL,
    AT_FOTASTOP,
    AT_FOTAGETDIFF,
#endif

    AT_KVGET,
    AT_KVSET,
    AT_KVDEL,
    AT_KVGETINT,
    AT_KVSETINT,
    AT_KVDELINT,

#if defined(CONFIG_CLOUDIO_ALICOAP)
    AT_ALICOAP_PUB,
    AT_ALICOAP_CONN,
    AT_ALICOAP_DISCONN,
#elif defined(CONFIG_CLOUDIO_ALIMQTT)
    AT_ALIMQTT_PUB,
    AT_ALIMQTT_CONN,
    AT_ALIMQTT_DISCONN,
#endif

#if defined(CONFIG_CLOUDIO_ONENET)
    AT_MIPLDELETE,
    AT_MIPLCREATE,
    AT_MIPLOPEN,
    AT_MIPLADDOBJ,
    AT_MIPLDELOBJ,
    AT_MIPLCLOSE,
    AT_MIPLNOTIFY,
    AT_MIPLREADRSP,
    AT_MIPLWRITERSP,
    AT_MIPLEXECUTERSP,
    AT_MIPLOBSERVERSP,
    AT_MIPLDISCOVERRSP,
    AT_MIPLPARAMETERRSP,
    AT_MIPLUPDATE,
    AT_MIPLVER,
    AT_COPREG,
#endif
    AT_CIPSTART,
    AT_CIPSTOP,
    AT_CIPRECVCFG,
    AT_CIPID,
    AT_CIPSTATUS,
    AT_CIPSEND,
#ifdef CONFIG_YOC_LPM
    AT_CIPSENDPSM,
#endif
    AT_CIPRECV,

#if defined (CONFIG_AT_SOCKET_ZTW)
    AT_ZIPOPEN,
    AT_ZIPSEND,
    AT_ZIPCLOSE,
    AT_ZIPSTAT,
    AT_ZDTMODE,
#endif

#if defined (CONFIG_AT_SOCKET_HISI)
    AT_NSOCR,
    AT_NSOST,
    AT_NSOSTF,
    AT_NSORF,
    AT_NSOCL,
#endif

#if defined(CONFIG_CHIP_ZX297100)
    AT_ZNVSET,
    AT_ZNVGET,
    AT_RAMDUMP,
#ifdef CONFIG_YOC_LPM
    AT_EXTRTC,
    AT_LPMENTER,
#endif
#ifdef CONFIG_AMT
    AT_ZFLAG,
    AT_AMTDEMO,
    AT_BOARDNUM,
    AT_MSN,
    AT_PRODTEST,
    AT_RTESTINFO,
    AT_ZVERSIONTYPE,
    AT_PLATFORM,
#endif
#endif

    AT_NULL,
};
#endif
