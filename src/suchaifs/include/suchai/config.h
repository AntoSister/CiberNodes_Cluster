/**
 * @file  config.h
 *
 * @author Carlos Gonzalez C
 * @author Camilo Rojas M
 * @author Tomas Opazo T
 * @author Tamara Gutierrez R
 * @author Matias Ramirez M
 * @author Ignacio Ibanez A
 * @author Diego Ortego P
 *
 * @date 2021
 * @copyright GNU GPL v3
 *
 * This header contains system wide settings to customize different submodules
 */

#ifndef SUCHAI_CONFIG_H
#define	SUCHAI_CONFIG_H

/* Select one operating system */
#define LINUX                       1
/* #undef FREERTOS */
/* #undef SIM */
#define SCH_OS                      LINUX  ///< Operating system port | FREERTOS
/* Select the correct architecture */
#define X86                         10
/* #undef RPI */
/* #undef ESP32 */
/* #undef NANOMIND */
#define SCH_ARCH                    X86  ///< Hardware port X86 | RPI | ESP32 | NANOMIND
#define SCH_HAVE_MALLOC
/* Select the correct app */
/* #undef SCH_APP */

/* System debug configurations */
#define SCH_LOG_LEVEL               LOG_LVL_INFO  ///< LOG_LVL_INFO |  LOG_LVL_DEBUG
#define SCH_NAME                    "SUCHAI-FS"  ///< Project code name
#define SCH_DEVICE_ID               1  ///< Device unique ID
#define SCH_SW_VERSION              "3.0.0.rc-2-25-g1c59"  ///< Software version

/* General system settings, enable/disable core tasks */
#define SCH_CON_ENABLED 1
#define SCH_COMM_ENABLE 1
#define SCH_FP_ENABLED 1
#define SCH_HK_ENABLED 0
#define SCH_HOOK_INIT
/* #undef SCH_HOOK_COMM */
#define SCH_WDT_PERIOD              120  ///< External WDT period
#define SCH_MAX_WDT_TIMER           60  ///< Seconds to send wdt_reset command
#define SCH_MAX_GND_WDT_TIMER       48*3600  ///< Seconds to reset the OBC if the ground watchdog was not clear
#define SCH_UART_BAUDRATE           (500000)  ///< UART baud rate for serial console
#define SCH_KISS_UART_BAUDRATE      (500000)  ///< UART baud rate for kiss communication
#define SCH_KISS_DEVICE             "/dev/ttyUSB0"  ///< Kiss device path

/* Communications system settings */
#define SCH_TRX_PORT_FILE            (9)   ///< Files port
#define SCH_TRX_PORT_TC              (10)  ///< Telecommands port
#define SCH_TRX_PORT_RPT             (11)  ///< Digirepeater port (resend packets)
#define SCH_TRX_PORT_CMD             (12)  ///< Commands port (execute console commands)
#define SCH_TRX_PORT_DBG             (13)  ///< Debug port, logs output
#define SCH_TRX_PORT_DBG_TM          (14)  ///< Debug telemetry port, logs frames
#define SCH_TRX_PORT_TM              (15)  ///< Telemetry port
#define SCH_TRX_PORT_APP                  15  ///< Apps telemetries starting port
#define SCH_COMM_NODE                1  ///< Node address
#define SCH_COM_MAX_PACKETS          10  /// Max number of packets to transmit in a row before a small pause
#define SCH_COM_TX_DELAY_MS          3000  /// Delay (ms) between continuous transmissions
#define SCH_CSP_BUFFERS              100  ///< Number of available CSP buffers
#define SCH_CSP_SOCK_LEN             100  ///< Max number of packets in a connection queue
#define SCH_COMM_ZMQ_IN              "tcp://127.0.0.1:8001"  ///< CSP ZMQ In socket URI
#define SCH_COMM_ZMQ_OUT             "tcp://127.0.0.1:8002"  ///< CSP ZMQ Out socket URI
#define SCH_CSP_CONN_TIMEOUT         1000  ///< CSP connection accept timeout
#define SCH_CSP_READ_TIMEOUT         100  ///< CSP connection read timeout

/* Data repository settings */
#define SCH_ST_RAM                   0
/* #undef SCH_ST_SQLITE */
/* #undef SCH_ST_POSTGRES */
/* #undef SCH_ST_FLASH */
#define SCH_STORAGE_MODE             SCH_ST_RAM  ///< Status repository location. (0) RAM, (1) Single external.
/* #undef SCH_STORAGE_TRIPLE_WR */
#define SCH_STORAGE_FILE             "/tmp/suchai.db"  ///< File to store the database, only if @SCH_STORAGE_MODE is SCH_ST_SQLITE
/* #undef SCH_STORAGE_PGUSER */
/* #undef SCH_STORAGE_PGPASS */
/* #undef SCH_STORAGE_PGHOST */

#define SCH_CMD_QUEUE_LEN                 50  ///< Commands queue length
#define SCH_FP_MAX_ENTRIES           (255)  ///< Max number of flight plan entries
#define SCH_CMD_MAX_ENTRIES          (255)  ///< Max number of commands in the repository
#define SCH_SECTIONS_PER_PAYLOAD     (10)  ///< Memory blocks for storing each payload type TODO: Make configurable per payload
#define SCH_SIZE_PER_SECTION         (256*1024)  ///< Size of each memory block in flash storage
#define SCH_FLASH_INIT_MEMORY        0*SCH_SIZE_PER_SECTION  ///< Initial address in flash storage

/**
 * Memory settings.
 *
 * Control the memory used by task stacks, static allocated buffers, etc.
 * Note that in FreeRTOS the stack size is measured in words not bytes, so the
 * final stack size depends on the architecture stack wide
 * (@see https://www.freertos.org/a00125.html)
 */
#define SCH_TASK_DEF_STACK            (5*256)  ///< Default task stack size in words
#define SCH_TASK_DIS_STACK            (5*256)  ///< Dispatcher task stack size in words
#define SCH_TASK_EXE_STACK            (10*256)  ///< Executer task stack size in words
#define SCH_TASK_WDT_STACK            (5*256)  ///< Watchdog task stack size in words
#define SCH_TASK_INI_STACK            (7*256)  ///< Init task stack size in words
#define SCH_TASK_COM_STACK            (5*256)  ///< Communications task stack size in words
#define SCH_TASK_FPL_STACK            (5*256)  ///< Flight plan task stack size in words
#define SCH_TASK_CON_STACK            (5*256)  ///< Console task stack size in words
#define SCH_TASK_CSP_STACK            (5*256)  ///< CSP route task stack size in words

#define SCH_BUFF_MAX_LEN              (256)  ///< General buffers max length in bytes
#define SCH_CMD_MAX_STR_PARAMS        (248)  ///< Limit for the parameters length
#define SCH_CMD_MAX_STR_NAME          (248)  ///< Limit for the length of the name of a command
#define SCH_CMD_MAX_STR_FORMAT        (248)  ///< Limit for the length of the format field of a command

#endif //SUCHAI_CONFIG_H
