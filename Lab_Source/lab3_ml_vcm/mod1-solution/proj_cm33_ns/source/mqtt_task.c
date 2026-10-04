/******************************************************************************
* File Name:   mqtt_task.c
*
* Description: This file contains the task that handles initialization &
*              connection of Wi-Fi and the MQTT client on CM33, the primary
*              connectivity core.
*
*              The task does the followings:
*              - Shares the MQTT connection with CM55 through the Virtual
*              Connectivity Manager (VCM).
*              - Starts the subscriber task.
*              - Implements reconnection mechanisms to handle WiFi and MQTT
*              disconnections.
*              - Handles all the cleanup operations to gracefully
*              terminate the Wi-Fi and MQTT connections in case of any failure.
*
* Related Document: See README.md
*
*******************************************************************************
 * (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"
#include "ipc_def.h"

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Task header files */
#include "mqtt_task.h"
#include "subscriber_task.h"

/* Configuration file for Wi-Fi and MQTT client */
#include "wifi_config.h"
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "retarget_io_init.h"
#include "cy_wcm.h"
#include "cy_mqtt_api.h"
#include "clock.h"
#include "safe_log.h"

/* LwIP header files */
#include "lwip/netif.h"

/******************************************************************************
* Macros
******************************************************************************/
/* Queue length of a message queue that is used to communicate the status of
 * various operations.
 */
#define MQTT_TASK_QUEUE_LENGTH           (3U)

/* Descriptor used by CM55 to get the handle of this MQTT connection. */
#define MQTT_HANDLE_DESCRIPTOR           "MQTThandleID"

/* Flag Masks for tracking which cleanup functions must be called. */
#define WCM_INITIALIZED                  (1lu << 0)
#define WIFI_CONNECTED                   (1lu << 1)
#define LIBS_INITIALIZED                 (1lu << 2)
#define BUFFER_INITIALIZED               (1lu << 3)
#define MQTT_INSTANCE_CREATED            (1lu << 4)
#define MQTT_CONNECTION_SUCCESS          (1lu << 5)
#define MQTT_MSG_RECEIVED                (1lu << 6)

#define TIME_DIV_MS                      (60000U)
#define APP_SDIO_INTERRUPT_PRIORITY      (7U)
#define APP_HOST_WAKE_INTERRUPT_PRIORITY (2U)
#define APP_SDIO_FREQUENCY_HZ            (25000000U)
#define SDHC_SDIO_64BYTES_BLOCK          (64U)
#define USERNAME_PRESENT                 (0U)
#define OFFSET_COUNT                     (1U)
#define NO_RETRIES                       (0U)
#define SNPRINTF_ERROR                   (0)
#define WIFI_NOT_CONNECTED               (0U)

/* Macro to check if the result of an operation was successful and set the
 * corresponding bit in the status_flag based on 'init_mask' parameter. When
 * it has failed, print the error message and return the result to the
 * calling function.
 */
#define CHECK_RESULT(result, init_mask, error_message...)      \
                     do                                        \
                     {                                         \
                         if (CY_RSLT_SUCCESS == (int)result)   \
                         {                                     \
                             status_flag |= init_mask;         \
                         }                                     \
                         else                                  \
                         {                                     \
                             safe_print(error_message);        \
                             return result;                    \
                         }                                     \
                     } while(0)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT connection handle. */
cy_mqtt_t mqtt_connection;

/* Queue handle used to communicate results of various operations - MQTT
 * Subscribe, MQTT connection, and Wi-Fi connection between tasks and
 * callbacks.
 */
QueueHandle_t mqtt_task_q;

/* Flag to denote initialization status of various operations. */
static uint32_t status_flag;

/* Pointer to the network buffer needed by the MQTT library for MQTT send and
 * receive operations.
 */
static uint8_t *mqtt_network_buffer = NULL;
static mtb_hal_sdio_t sdio_instance;
static cy_stc_sd_host_context_t sdhc_host_context;
static cy_wcm_config_t wcm_config;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)

/* SysPm callback parameter structure for SDHC */
static cy_stc_syspm_callback_params_t sdcardDSParams =
{
    .context   = &sdhc_host_context,
    .base      = CYBSP_WIFI_SDIO_HW
};

/* SysPm callback structure for SDHC*/
static cy_stc_syspm_callback_t sdhcDeepSleepCallbackHandler =
{
    .callback           = Cy_SD_Host_DeepSleepCallback,
    .skipMode           = SYSPM_SKIP_MODE,
    .type               = CY_SYSPM_DEEPSLEEP,
    .callbackParams     = &sdcardDSParams,
    .prevItm            = NULL,
    .nextItm            = NULL,
    .order              = SYSPM_CALLBACK_ORDER
};
#endif /* (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) */

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static cy_rslt_t wifi_connect(void);
static cy_rslt_t mqtt_init(void);
static cy_rslt_t mqtt_connect(void);

static void sdio_interrupt_handler(void);
static void host_wake_interrupt_handler(void);
static void app_sdio_init(void);
static void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data);
static void cleanup(void);

#if GENERATE_UNIQUE_CLIENT_ID
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier);
#endif /* GENERATE_UNIQUE_CLIENT_ID */

/******************************************************************************
 * Function Name: mqtt_client_task
 ******************************************************************************
 * Summary:
 *  Task for handling initialization & connection of Wi-Fi and the MQTT client.
 *  Once connected, it signals CM55 that the MQTT connection can be shared and
 *  creates the subscriber task. The task also handles the WiFi and MQTT
 *  connections by initiating reconnection on the event of disconnections.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 ******************************************************************************/
void mqtt_client_task(void *pvParameters)
{
    cy_rslt_t result;

    /* Structures that store the data to be sent/received to/from various
     * message queues.
     */
    mqtt_task_cmd_t mqtt_status;
    subscriber_data_t subscriber_q_data;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Configure the Wi-Fi interface as a Wi-Fi STA (i.e. Client). */
    app_sdio_init();

    wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;
    wcm_config.wifi_interface_instance = &sdio_instance;

    /* Create a message queue to communicate with other tasks and callbacks. */
    mqtt_task_q = xQueueCreate(MQTT_TASK_QUEUE_LENGTH, sizeof(mqtt_task_cmd_t));

    /* Initialize the Wi-Fi Connection Manager. */
    if (CY_RSLT_SUCCESS != cy_wcm_init(&wcm_config))
    {
        handle_app_error();
    }

    /* Set the appropriate bit in the status_flag to denote successful
     * WCM initialization.
     */
    status_flag |= WCM_INITIALIZED;
    safe_print("\nWi-Fi Connection Manager initialized.\r\n");

    /* Initiate connection to the Wi-Fi AP and cleanup if the operation fails. */
    if (CY_RSLT_SUCCESS != wifi_connect())
    {
        cleanup();
        vTaskDelete(NULL);
    }

    /* Set-up the MQTT client and connect to the MQTT broker. Cleanup if any
     * of the operations fail.
     */
    if ( (CY_RSLT_SUCCESS != mqtt_init()) || (CY_RSLT_SUCCESS != mqtt_connect()) )
    {
        cleanup();
        vTaskDelete(NULL);
    }

    /* The MQTT connection is ready. Signal CM55 that it can get the MQTT
     * handle through the Virtual Connectivity Manager.
     */
    Cy_IPC_Sema_Set(MQTT_READY_SEMA_NUM, false);
    safe_print("MQTT connection shared with CM55.\r\n");

    /* Create the subscriber task and cleanup if the operation fails. */
    if (pdPASS != xTaskCreate(subscriber_task, "Subscriber task", SUBSCRIBER_TASK_STACK_SIZE,
                              NULL, SUBSCRIBER_TASK_PRIORITY, &subscriber_task_handle))
    {
        safe_print("Failed to create the Subscriber task!\r\n");
        cleanup();
        vTaskDelete(NULL);
    }

    while (true)
    {
        /* Wait for results of MQTT operations from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(mqtt_task_q, &mqtt_status, portMAX_DELAY))
        {
            /* In this code example, the disconnection from the MQTT Broker or
             * the Wi-Fi network is handled by the case 'HANDLE_DISCONNECTION'.
             *
             * The publish and subscribe failures (`HANDLE_MQTT_PUBLISH_FAILURE`
             * and `HANDLE_MQTT_SUBSCRIBE_FAILURE`) does not initiate
             * reconnection in this example, but they can be handled as per the
             * application requirement in the following switch cases.
             */
            switch(mqtt_status)
            {
                case HANDLE_MQTT_PUBLISH_FAILURE:
                {
                    /* Handle Publish Failure here. */
                    break;
                }

                case HANDLE_MQTT_SUBSCRIBE_FAILURE:
                {
                    /* Handle Subscribe Failure here. */
                    break;
                }

                case HANDLE_DISCONNECTION:
                {
                    /* Although the connection with the MQTT Broker is lost,
                     * call the MQTT disconnect API for cleanup of threads and
                     * other resources before reconnection.
                     */
                    result = cy_mqtt_disconnect(mqtt_connection);
                    if (CY_RSLT_SUCCESS != result)
                    {
                        safe_print("\nMQTT disconnection failed on CM33.\r\n");
                    }

                    /* Check if Wi-Fi connection is active. If not, update the
                     * status flag and initiate Wi-Fi reconnection.
                     */
                    if (WIFI_NOT_CONNECTED == cy_wcm_is_connected_to_ap())
                    {
                        status_flag &= ~(WIFI_CONNECTED);
                        safe_print("\nInitiating Wi-Fi Reconnection...\r\n");
                        if (CY_RSLT_SUCCESS != wifi_connect())
                        {
                            cleanup();
                            vTaskDelete(NULL);
                        }
                    }

                    safe_print("\nInitiating MQTT Reconnection...\r\n");
                    if (CY_RSLT_SUCCESS != mqtt_connect())
                    {
                        cleanup();
                        vTaskDelete(NULL);
                    }

                    /* Initiate MQTT subscribe post the reconnection. */
                    subscriber_q_data.cmd = SUBSCRIBE_TO_TOPIC;
                    xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);
                    break;
                }

                default:
                    break;
            }
        }
    }
}

/*******************************************************************************
* Function Name: app_sdio_init
********************************************************************************
* Summary:
* This function configures and initializes the SDIO instance used in
* communication between the host MCU and the wireless device.
*
* Parameters:
*  void
*
* Return:
*  void
*******************************************************************************/
static void app_sdio_init(void)
{
    cy_rslt_t result;
    mtb_hal_sdio_cfg_t sdio_hal_cfg;
    cy_stc_sysint_t sdio_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = APP_SDIO_INTERRUPT_PRIORITY
    };

    cy_stc_sysint_t host_wake_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_HOST_WAKE_IRQ,
        .intrPriority = APP_HOST_WAKE_INTERRUPT_PRIORITY
    };

    /* Initialize the SDIO interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = Cy_SysInt_Init(&sdio_intr_cfg, sdio_interrupt_handler);

    /* SDIO interrupt initialization failed. Stop program execution. */
    if (CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    /* Setup SDIO using the HAL object and desired configuration */
    result = mtb_hal_sdio_setup(&sdio_instance, &CYBSP_WIFI_SDIO_sdio_hal_config, NULL, &sdhc_host_context);

    /* SDIO setup failed. Stop program execution. */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Initialize and Enable SD HOST */
    Cy_SD_Host_Enable(CYBSP_WIFI_SDIO_HW);
    Cy_SD_Host_Init(CYBSP_WIFI_SDIO_HW, CYBSP_WIFI_SDIO_sdio_hal_config.host_config, &sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(CYBSP_WIFI_SDIO_HW, CY_SD_HOST_BUS_WIDTH_4_BIT);

    sdio_hal_cfg.frequencyhal_hz = APP_SDIO_FREQUENCY_HZ;
    sdio_hal_cfg.block_size = SDHC_SDIO_64BYTES_BLOCK;

    /* Configure SDIO */
    mtb_hal_sdio_configure(&sdio_instance, &sdio_hal_cfg);

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
    /* SDHC SysPm callback registration */
    Cy_SysPm_RegisterCallback(&sdhcDeepSleepCallbackHandler);
#endif /* (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) */

    /* Setup GPIO using the HAL object for WIFI WL REG ON  */
    mtb_hal_gpio_setup(&wcm_config.wifi_wl_pin, CYBSP_WIFI_WL_REG_ON_PORT_NUM, CYBSP_WIFI_WL_REG_ON_PIN);

    /* Setup GPIO using the HAL object for WIFI HOST WAKE PIN  */
    mtb_hal_gpio_setup(&wcm_config.wifi_host_wake_pin, CYBSP_WIFI_HOST_WAKE_PORT_NUM, CYBSP_WIFI_HOST_WAKE_PIN);

    /* Initialize the Host wakeup interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status_host_wake = Cy_SysInt_Init(&host_wake_intr_cfg, host_wake_interrupt_handler);

    /* Host wake up interrupt initialization failed. Stop program execution. */
    if (CY_SYSINT_SUCCESS != interrupt_init_status_host_wake)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);
}

/******************************************************************************
 * Function Name: wifi_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates connection to the Wi-Fi Access Point using the
 *  specified SSID and PASSWORD. The connection is retried a maximum of
 *  'MAX_WIFI_CONN_RETRIES' times with interval of 'WIFI_CONN_RETRY_INTERVAL_MS'
 *  milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful Wi-Fi connection, else an
 *              error code indicating the failure.
 ******************************************************************************/
static cy_rslt_t wifi_connect(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;

    /* Check if Wi-Fi connection is already established. */
    if (WIFI_NOT_CONNECTED == cy_wcm_is_connected_to_ap())
    {
        /* Configure the connection parameters for the Wi-Fi interface. */
        memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
        memcpy(connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
        memcpy(connect_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
        connect_param.ap_credentials.security = WIFI_SECURITY;

        safe_print("\nWi-Fi Connecting to '%s'\r\n", connect_param.ap_credentials.SSID);

        /* Connect to the Wi-Fi AP. */
        for (uint32_t retry_count = NO_RETRIES; retry_count < MAX_WIFI_CONN_RETRIES; retry_count++)
        {
            result = cy_wcm_connect_ap(&connect_param, &ip_address);

            if (CY_RSLT_SUCCESS == result)
            {
                safe_print("\nSuccessfully connected to Wi-Fi network '%s'.\r\n",
                           connect_param.ap_credentials.SSID);

                /* Set the appropriate bit in the status_flag to denote
                 * successful Wi-Fi connection, print the assigned IP address.
                 */
                status_flag |= WIFI_CONNECTED;
                if (CY_WCM_IP_VER_V4 == ip_address.version)
                {
                    safe_print("IPv4 Address Assigned: %s\r\n",
                               ip4addr_ntoa((const ip4_addr_t *) &ip_address.ip.v4));
                }
                else if (CY_WCM_IP_VER_V6 == ip_address.version)
                {
                    safe_print("IPv6 Address Assigned: %s\r\n",
                               ip6addr_ntoa((const ip6_addr_t *) &ip_address.ip.v6));
                }
                return result;
            }

            safe_print("Wi-Fi Connection failed. Error code:0x%0X. Retrying in %d ms. Retries left: %d\r\n",
                       (int)result, WIFI_CONN_RETRY_INTERVAL_MS,
                       (int)(MAX_WIFI_CONN_RETRIES - retry_count - 1));
            vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MS));
        }

        safe_print("\nExceeded maximum Wi-Fi connection attempts!\r\n");
        safe_print("Wi-Fi connection failed after retrying for %d mins\r\n",
                   (int)(WIFI_CONN_RETRY_INTERVAL_MS * MAX_WIFI_CONN_RETRIES) / TIME_DIV_MS);
    }
    return result;
}

/******************************************************************************
 * Function Name: mqtt_init
 ******************************************************************************
 * Summary:
 *  Function that initializes the MQTT library and creates an instance for the
 *  MQTT client. The network buffer needed by the MQTT library for MQTT send
 *  and receive operations is also allocated by this function.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS on a successful initialization, else an error
 *              code indicating the failure.
 ******************************************************************************/
static cy_rslt_t mqtt_init(void)
{
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the MQTT library. */
    result = cy_mqtt_init();
    CHECK_RESULT(result, LIBS_INITIALIZED, "\nMQTT library initialization failed!\r\n");

    /* Allocate buffer for MQTT send and receive operations. */
    mqtt_network_buffer = (uint8_t *) pvPortMalloc(sizeof(uint8_t) * MQTT_NETWORK_BUFFER_SIZE);
    if (NULL == mqtt_network_buffer)
    {
        result = ~CY_RSLT_SUCCESS;
    }
    CHECK_RESULT(result, BUFFER_INITIALIZED, "\nNetwork Buffer allocation failed!\r\n");

    /* Create the MQTT client instance. The descriptor allows CM55 to get the
     * handle of this MQTT connection.
     */
    result = cy_mqtt_create(mqtt_network_buffer, MQTT_NETWORK_BUFFER_SIZE,
                            security_info, &broker_info, MQTT_HANDLE_DESCRIPTOR,
                            &mqtt_connection);
    CHECK_RESULT(result, MQTT_INSTANCE_CREATED, "\nMQTT instance creation failed!\r\n");

    /* Register a MQTT event callback */
    result = cy_mqtt_register_event_callback(mqtt_connection,
                                             (cy_mqtt_callback_t)mqtt_event_callback, NULL);
    if (CY_RSLT_SUCCESS == result)
    {
        safe_print("\nMQTT library initialization successful.\r\n");
    }
    return result;
}

/******************************************************************************
 * Function Name: mqtt_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates MQTT connect operation. The connection is retried
 *  a maximum of 'MAX_MQTT_CONN_RETRIES' times with interval of
 *  'MQTT_CONN_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful MQTT connection, else an
 *              error code indicating the failure.
 ******************************************************************************/
static cy_rslt_t mqtt_connect(void)
{
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* MQTT client identifier string. */
    char mqtt_client_identifier[(MQTT_CLIENT_IDENTIFIER_MAX_LEN + OFFSET_COUNT)] = MQTT_CLIENT_IDENTIFIER;

    /* Configure the user credentials as a part of MQTT Connect packet */
    if (strlen(MQTT_USERNAME) > USERNAME_PRESENT)
    {
        connection_info.username = MQTT_USERNAME;
        connection_info.password = MQTT_PASSWORD;
        connection_info.username_len = sizeof(MQTT_USERNAME) - OFFSET_COUNT;
        connection_info.password_len = sizeof(MQTT_PASSWORD) - OFFSET_COUNT;
    }

    /* Generate a unique client identifier with 'MQTT_CLIENT_IDENTIFIER' string
     * as a prefix if the `GENERATE_UNIQUE_CLIENT_ID` macro is enabled.
     */
#if GENERATE_UNIQUE_CLIENT_ID
    result = mqtt_get_unique_client_identifier(mqtt_client_identifier);
#endif /* GENERATE_UNIQUE_CLIENT_ID */

    /* Set the client identifier buffer and length. */
    connection_info.client_id = mqtt_client_identifier;
    connection_info.client_id_len = strlen(mqtt_client_identifier);

    safe_print("\n'%.*s' connecting to MQTT broker '%.*s'...\r\n",
               connection_info.client_id_len,
               connection_info.client_id,
               broker_info.hostname_len,
               broker_info.hostname);

    for (uint32_t retry_count = NO_RETRIES; retry_count < MAX_MQTT_CONN_RETRIES; retry_count++)
    {
        if (WIFI_NOT_CONNECTED == cy_wcm_is_connected_to_ap())
        {
            safe_print("\nUnexpectedly disconnected from Wi-Fi network! \nInitiating Wi-Fi reconnection...\r\n");
            status_flag &= ~(WIFI_CONNECTED);

            /* Initiate Wi-Fi reconnection. */
            result = wifi_connect();
            if (CY_RSLT_SUCCESS != result)
            {
                return result;
            }
        }

        /* Establish the MQTT connection. */
        result = cy_mqtt_connect(mqtt_connection, &connection_info);

        if (CY_RSLT_SUCCESS == result)
        {
            safe_print("MQTT connection successful.\r\n");

            /* Set the appropriate bit in the status_flag to denote successful
             * MQTT connection, and return the result to the calling function.
             */
            status_flag |= MQTT_CONNECTION_SUCCESS;
            return result;
        }

        safe_print("\nMQTT connection failed with error code 0x%0X. \nRetrying in %d ms. Retries left: %d\r\n",
                   (int)result, MQTT_CONN_RETRY_INTERVAL_MS,
                   (int)(MAX_MQTT_CONN_RETRIES - retry_count - OFFSET_COUNT));
        vTaskDelay(pdMS_TO_TICKS(MQTT_CONN_RETRY_INTERVAL_MS));
    }

    safe_print("\nExceeded maximum MQTT connection attempts\r\n");
    safe_print("MQTT connection failed after retrying for %d mins\r\n",
               (int)(MQTT_CONN_RETRY_INTERVAL_MS * MAX_MQTT_CONN_RETRIES) / TIME_DIV_MS);
    return result;
}

/******************************************************************************
 * Function Name: mqtt_event_callback
 ******************************************************************************
 * Summary:
 *  Callback invoked by the MQTT library for events like MQTT disconnection,
 *  incoming MQTT subscription messages from the MQTT broker.
 *    1. In case of MQTT disconnection, the MQTT client task is communicated
 *       about the disconnection using a message queue.
 *    2. When an MQTT subscription message is received, the subscriber callback
 *       function implemented in subscriber_task.c is invoked to handle the
 *       incoming MQTT message.
 *
 * Parameters:
 *  cy_mqtt_t mqtt_handle : MQTT handle corresponding to the MQTT event (unused)
 *  cy_mqtt_event_t event : MQTT event information
 *  void *user_data : User data pointer passed during cy_mqtt_create() (unused)
 *
 * Return:
 *  void
 ******************************************************************************/
static void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data)
{
    cy_mqtt_publish_info_t *received_msg;
    mqtt_task_cmd_t mqtt_task_cmd;

    (void) mqtt_handle;
    (void) user_data;

    switch(event.type)
    {
        case CY_MQTT_EVENT_TYPE_DISCONNECT:
        {
            /* Clear the status flag bit to indicate MQTT disconnection. */
            status_flag &= ~(MQTT_CONNECTION_SUCCESS);

            /* MQTT connection with the MQTT broker is broken as the client
             * is unable to communicate with the broker. Send the message to
             * the MQTT client task to handle the disconnection.
             */
            safe_print("\nUnexpectedly disconnected from MQTT broker!\r\n");
            mqtt_task_cmd = HANDLE_DISCONNECTION;
            xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
            break;
        }

        case CY_MQTT_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECEIVE:
        {
            status_flag |= MQTT_MSG_RECEIVED;

            /* Incoming MQTT message has been received. Send this message to
             * the subscriber callback function to handle it.
             */
            received_msg = &(event.data.pub_msg.received_message);
            mqtt_subscription_callback(received_msg);
            break;
        }

        default :
        {
            /* Unknown MQTT event */
            safe_print("\nUnknown Event received from MQTT callback!\r\n");
            break;
        }
    }
}

#if GENERATE_UNIQUE_CLIENT_ID
/******************************************************************************
 * Function Name: mqtt_get_unique_client_identifier
 ******************************************************************************
 * Summary:
 *  Function that generates unique client identifier for the MQTT client by
 *  appending a timestamp to a common prefix 'MQTT_CLIENT_IDENTIFIER'.
 *
 * Parameters:
 *  char *mqtt_client_identifier : Pointer to the string that stores the
 *                                 generated unique identifier
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS on successful generation of the client
 *              identifier, else a non-zero value indicating failure.
 ******************************************************************************/
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    /* Check for errors from snprintf. */
    if (snprintf(mqtt_client_identifier,
                 (MQTT_CLIENT_IDENTIFIER_MAX_LEN + OFFSET_COUNT),
                 MQTT_CLIENT_IDENTIFIER "%lu",
                 (long unsigned int)Clock_GetTimeMs()) < SNPRINTF_ERROR)
    {
        status = ~CY_RSLT_SUCCESS;
    }

    return status;
}
#endif /* GENERATE_UNIQUE_CLIENT_ID */

/******************************************************************************
 * Function Name: cleanup
 ******************************************************************************
 * Summary:
 *  Function that invokes the deinit and cleanup functions for various
 *  operations based on the status_flag.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 ******************************************************************************/
static void cleanup(void)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    safe_print("\nTerminating Subscriber task...\r\n");
    if (NULL != subscriber_task_handle)
    {
        vTaskDelete(subscriber_task_handle);
    }

    /* Disconnect the MQTT connection if it was established. */
    if (status_flag & MQTT_CONNECTION_SUCCESS)
    {
        status = cy_mqtt_disconnect(mqtt_connection);
        safe_print((CY_RSLT_SUCCESS == status) ? "Disconnected from the MQTT Broker...\r\n" :
                                                 "MQTT disconnect API failed unexpectedly.\r\n");
    }

    /* Delete the MQTT instance if it was created. */
    if (status_flag & MQTT_INSTANCE_CREATED)
    {
        status = cy_mqtt_delete(mqtt_connection);
        safe_print((CY_RSLT_SUCCESS == status) ? "Removed MQTT connection info from stack...\r\n" :
                                                 "MQTT delete API failed unexpectedly.\r\n");
    }

    /* Deallocate the network buffer. */
    if (status_flag & BUFFER_INITIALIZED)
    {
        vPortFree((void *) mqtt_network_buffer);
    }

    /* Deinit the MQTT library. */
    if (status_flag & LIBS_INITIALIZED)
    {
        status = cy_mqtt_deinit();
        safe_print((CY_RSLT_SUCCESS == status) ? "Deinitialized MQTT stack...\r\n" :
                                                 "MQTT deinit API failed unexpectedly.\r\n");
    }

    /* Disconnect from Wi-Fi AP. */
    if (status_flag & WIFI_CONNECTED)
    {
        status = cy_wcm_disconnect_ap();
        safe_print((CY_RSLT_SUCCESS == status) ? "Disconnected from the Wi-Fi AP!\r\n" :
                                                 "WCM disconnect AP failed unexpectedly.\r\n");
    }

    /* De-initialize the Wi-Fi Connection Manager. */
    if (status_flag & WCM_INITIALIZED)
    {
        status = cy_wcm_deinit();
        safe_print((CY_RSLT_SUCCESS == status) ? "Deinitialized Wifi connection...\r\n" :
                                                 "WCM deinit API failed unexpectedly.\r\n");
    }
}

/*******************************************************************************
* Function Name: sdio_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for SDIO instance.
*******************************************************************************/
static void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&sdio_instance);
}

/*******************************************************************************
* Function Name: host_wake_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for the host wake up input pin.
*******************************************************************************/
static void host_wake_interrupt_handler(void)
{
    mtb_hal_gpio_process_interrupt(&wcm_config.wifi_host_wake_pin);
}

/* [] END OF FILE */
