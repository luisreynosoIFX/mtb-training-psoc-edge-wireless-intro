/*******************************************************************************
 * File Name:   wifi_task.c
 *
 * Description: This file contains the task definition for initializing the
 * Wi-Fi device, connecting to the AP, disconnecting from AP, scanning and 
 * EEPROM related functionality.
 *
 * Related Document: See Readme.md
 *
 *******************************************************************************
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
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

#include <FreeRTOS.h>
#include <task.h>
#include "cybsp.h"
#include "stdio.h"
#include "cy_wcm.h"
#include "wifi_task.h"
#include "cycfg_gatt_db.h"
#include "wiced_bt_stack.h"
#include "app_utils.h"
#include "cycfg_gap.h"
#include "wiced_memory.h"
#include "retarget_io_init.h"

#define SCAN_DATA_HEADER                            (2U)
#define MASK_8BITS                                  0x000000ff
#define MASK_16BITS                                 0x0000ff00
#define MASK_24BITS                                 0x00ff0000
#define MASK_32BITS                                 0xff000000
#define SHIFT_8BITS                                  (8U)
#define SHIFT_16BITS                                 (16U)
#define SHIFT_24BITS                                 (24U)
#define APP_SDIO_INTERRUPT_PRIORITY                  (7U)
#define APP_HOST_WAKE_INTERRUPT_PRIORITY             (2U)
#define APP_SDIO_FREQUENCY_HZ                        (25000000U)
#define SDHC_SDIO_64BYTES_BLOCK                      (64U)



/******************************************************************************
 * Global Variables
 ******************************************************************************/
/* WCM related structures */
cy_wcm_connect_params_t wifi_conn_param;

/* Variable to be used to store the WiFi credentials which can be copied to
 * NVRAM
 */
wifi_details_t wifi_details;

/******************************************************************************
 * Extern Functions and Variables
 ******************************************************************************/
/* Maintains the connection id of the current connection */
extern uint16_t conn_id;

/* This variable is set to true when button callback is received and
 * data is present in NVM. It is set to false after the WiFi Task
 * processes Disconnection notification. It is used to check button
 * interrupt while the device is trying to connect to WiFi
 */
extern bool button_pressed;

/* Task Handles for WiFi Task */
extern TaskHandle_t wifi_task_handle;
static mtb_hal_sdio_t sdio_instance;
cy_stc_sd_host_context_t sdhc_host_context;
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
#endif
/*******************************************************************************
* Function Definitions
*******************************************************************************/

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

/*******************************************************************************
* Function Name: app_sdio_init
********************************************************************************
* Summary:
* This function configures and initializes the SDIO instance used in
* communication between the host MCU and the wireless device.
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
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    /* Setup SDIO using the HAL object and desired configuration */
    result = mtb_hal_sdio_setup(&sdio_instance, &CYBSP_WIFI_SDIO_sdio_hal_config, NULL, &sdhc_host_context);

    /* SDIO setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
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
    cy_en_sysint_status_t interrupt_init_status_host_wake =  Cy_SysInt_Init(&host_wake_intr_cfg, host_wake_interrupt_handler);

    /* Host wake up interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status_host_wake)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);
}

/*******************************************************************************
* Function Name: wifi_task
********************************************************************************
* Summary:
* This function initializes the WCM module and handles task notifications
*
* Parameters
* void *
*
* Return
* void
*******************************************************************************/
void wifi_task(void * arg)
{
    cy_rslt_t result ;

    /* Variable to use for filtering WiFi scan results */
    cy_wcm_scan_filter_t scan_filter;

    /* Notification values received from other tasks */
    uint32_t ulNotifiedValue;

    /* Return status for NVM. */
    cy_rslt_t nvm_result ;

    /* WCM configuration and IP address variables */
    cy_wcm_ip_address_t ip_address;

    app_sdio_init();

    /* Initialize wcm */
    wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;
    wcm_config.wifi_interface_instance = &sdio_instance;

    /* Initialize WCM */
    printf("\n");
    result = cy_wcm_init(&wcm_config);
    printf("\n");
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    while(true)
    {
        /* Wait for a notification */
        xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on
                                       entry. */
                         UINT32_MAX, /* Reset the notification value to 0 on
                                        exit. */
                         &ulNotifiedValue, /* Notified value pass out in
                                              ulNotifiedValue. */
                         portMAX_DELAY );  /* Block indefinitely. */

        /* Check if the notification value has the scan bit set. In some cases both
         * the scan bit and connect bit are set. In both the cases we start with
         * the scan, pass the notification value to the scan callback and if
         * the connect bit is also set then the scan callback will pass the
         * connect message to this task and this task will try and connect to
         * the WiFi network. If the notification value passed to the scan
         * callback contains only the scan bit then it will try and send
         * GATT notifications to the BLE client
         */
        if(NOTIF_SCAN & ulNotifiedValue)
        {
            memset(&scan_filter, 0, sizeof(cy_wcm_scan_filter_t));

            /* If the Connect bit is set then we need to only scan for networks
             * with the given SSID. Set the correct filter values and start scan
             */
            if(NOTIF_CONNECT & ulNotifiedValue)
            {
                /* Configure the scan filter for SSID */
                scan_filter.mode = CY_WCM_SCAN_FILTER_TYPE_SSID;
                memcpy((char *)scan_filter.param.SSID,(char *)wifi_conn_param.ap_credentials.SSID,
                strlen((char *)wifi_conn_param.ap_credentials.SSID) + 1);
                printf("Starting scan with SSID: %s\n", scan_filter.param.SSID);

                result = cy_wcm_start_scan(scan_callback, &ulNotifiedValue,
                                           &scan_filter);
            }
            /* Else start scan without any filter */
            else
            {
                result = cy_wcm_start_scan(scan_callback, &ulNotifiedValue, NULL);
            }

            if(result)
            {
                printf("Start scan failed\n");
            }
        }

        /* Check for connection notification */
        else if(NOTIF_CONNECT == ulNotifiedValue)
        {
            /* Variable to track the number of connection retries to the Wi-Fi
             * AP
             */
            uint8_t conn_retries = 0;

            /* Join the Wi-Fi AP */
            result = CY_RSLT_TYPE_ERROR;

            while((CY_RSLT_SUCCESS != result) && (conn_retries <
                                                        MAX_CONNECTION_RETRIES))
            {
                /* If button ISR is received then skip connecting to WiFi */
                if(button_pressed)
                {
                    break;
                }
                printf("\nTrying to connect SSID: %s, Password: %s\n",
                        wifi_conn_param.ap_credentials.SSID,
                        wifi_conn_param.ap_credentials.password);

                /* Connect to the given AP */
                result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);
                conn_retries++;
            }

            if(CY_RSLT_SUCCESS == result)
            {
                printf("Successfully joined the Wi-Fi network\n");
                Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, CYBSP_LED_STATE_ON);

                /* Update GATT DB about connection */
                app_custom_service_wifi_control[0] = WIFI_CONTROL_CONNECT;

                /* Check if the connection is active and  notifications are
                 * enabled and then send the notification
                 */
                if ((conn_id) &&
                   ((app_custom_service_wifi_control_client_char_config[0] &
                     GATT_CLIENT_CONFIG_NOTIFICATION)))
                {
                    uint8_t *p_attr = (uint8_t*)app_custom_service_wifi_control;
                    wiced_bt_gatt_server_send_notification(conn_id,
                                    HDLC_CUSTOM_SERVICE_WIFI_CONTROL_VALUE,
                                    sizeof(app_custom_service_wifi_control[0]),
                                    p_attr, NULL);
                }
                else /* Notification not sent */
                {
                    printf("Notification not sent\n");
                }
            }
            else /* WiFi connection failed */
            {
                /* Update GATT DB about unsuccessful connection */
                app_custom_service_wifi_control[0] = WIFI_CONTROL_DISCONNECT;

                /* Send notification for unsuccessful connection */
                /* Check if the connection is active and notifications are
                 * enabled and then send the notification
                 */
                if ((conn_id) &&
                    ((app_custom_service_wifi_control_client_char_config[0] &
                      GATT_CLIENT_CONFIG_NOTIFICATION)))
                {
                    uint8_t *p_attr = (uint8_t*)app_custom_service_wifi_control;
                    wiced_bt_gatt_server_send_notification(conn_id,
                                     HDLC_CUSTOM_SERVICE_WIFI_CONTROL_VALUE,
                                     sizeof(app_custom_service_wifi_control[0]),
                                     p_attr, NULL);
                }
                else /* Notification not sent */
                {
                    printf("Notification not sent\n");
                }

                printf("Failed to join Wi-Fi network\n");
                Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, CYBSP_LED_STATE_OFF);

                if((!conn_id) && (BTM_BLE_ADVERT_OFF ==
                                      wiced_bt_ble_get_current_advert_mode()))
                {
                    printf("Starting BLE ADV. Connect to BLE and provide"
                            "proper credentials\n");
                    /* Set the advertising params and make the device
                     * discoverable
                     */
                    result = wiced_bt_ble_set_raw_advertisement_data(
                                                     CY_BT_ADV_PACKET_DATA_SIZE,
                                                     cy_bt_adv_packet_data);
                    if(WICED_SUCCESS != result)
                    {
                        printf("Set ADV data failed\n");
                    }

                    wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_LOW,
                                                  0, NULL);
                }
            }
        }
        /* Task notification for disconnection */
        else if(NOTIF_DISCONNECT & ulNotifiedValue)
        {
            /* Check if the erase bit is also set in the notification. If yes
             * then erase the data from NVM
             */
            if(NOTIF_ERASE_DATA & ulNotifiedValue)
            {
                printf("Deleting Wi-Fi data from NVM\n");

                /* Set the data to 0*/
                memset(&wifi_details, 0, sizeof(wifi_details));
                nvm_result =  Cy_RRAM_WriteByteArray(RRAMC0, APP_RRAM_NVM_MAIN_NS_START , &wifi_details.wifi_ssid[0],sizeof(wifi_details));
                if (nvm_result)
                {
                    printf("Failed to write to NVM\n");
                }
            }

            printf("Disconnecting Wi-Fi\n");
            result = cy_wcm_disconnect_ap();
            if(CY_RSLT_SUCCESS == result)
            {
                /* Update GATT DB about disconnection */
                app_custom_service_wifi_control[0] = WIFI_CONTROL_DISCONNECT;

                printf("Successfully disconnected from AP\n");

                /* Start ADV if the device is not already connected or
                 * advertising
                */
                if((0 == conn_id) && (BTM_BLE_ADVERT_OFF ==
                                      wiced_bt_ble_get_current_advert_mode()))
                {
                    printf("Starting BLE ADV. Connect to BLE and provide "
                            "proper credentials\n");

                    result = wiced_bt_start_advertisements(
                                                  BTM_BLE_ADVERT_UNDIRECTED_LOW,
                                                  0, NULL);

                    if(WICED_SUCCESS != result)
                    {
                        printf("Failed to start ADV");
                    }
                }
                else /* Connection is active */
                {
                    /* Send notification for successful disconnection */
                    /* Check if the connection is active and notifications are
                     * enabled
                     */
                    if ((conn_id) &&
                        (app_custom_service_wifi_control_client_char_config[0] &
                         GATT_CLIENT_CONFIG_NOTIFICATION))
                    {
                        uint8_t *p_attr = (uint8_t*)app_custom_service_wifi_control;
                        wiced_bt_gatt_server_send_notification(conn_id,
                                     HDLC_CUSTOM_SERVICE_WIFI_CONTROL_VALUE,
                                     sizeof(app_custom_service_wifi_control[0]),
                                     p_attr, NULL);
                    }
                    else /* Notification not sent */
                    {
                        printf("Notification not sent\n");
                    }
                }
            }
            else /* Disconnection failed */
            {
                printf("Failed to disconnect\n");
            }

            /* Set the variable as false as the disconnection was done */
            button_pressed = false;
        }
    }
}

/*******************************************************************************
 * Function Name: scan_callback
 *******************************************************************************
 * Summary: The callback function which accumulates the scan results.
 * If the CONNECT bit is set in the user_data then send back task notification
 * to the WiFi task to connect to the AP. If not then send the results back to
 * the BLE Client
 *
 * Parameters:
 *  cy_wcm_scan_result_t *result_ptr: Pointer to the scan result
 *  void *user_data: User data.
 *  cy_wcm_scan_status_t status: Status of scan completion.
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void scan_callback(cy_wcm_scan_result_t *result_ptr, void *user_data,
                   cy_wcm_scan_status_t status)
{
    uint8_t *scan_data;
    uint8_t byte_no = 0;
    uint8_t ssid_len = 0;


    if(NULL != result_ptr)
    {
        ssid_len = strlen((const char *)result_ptr->SSID);
    }

    if ((ssid_len != 0) && (CY_WCM_SCAN_INCOMPLETE == status))
    {
        wifi_conn_param.ap_credentials.security = result_ptr->security;
            printf("%-32s\t", result_ptr->SSID);
            printf("%s\n", get_wifi_security_name(result_ptr->security));

        /* If scan bit is set then send the scan result to the BLE Client */
        if(NOTIF_SCAN == *(uint32_t *) user_data)
        {
            /* Get memory for the buffer to send the scan data */
            scan_data = (uint8_t *)wiced_bt_get_buffer(SCAN_DATA_HEADER + ssid_len + SCAN_DATA_HEADER + sizeof(uint32_t));
            if(NULL == scan_data)
            {
                printf("Buffer for notification not allocated\n");
                return;
            }
            /* Fill in the SSID first */
            scan_data[byte_no++] = SCAN_PACKET_TYPE_SSID;
            scan_data[byte_no++] = ssid_len;
            memcpy(&scan_data[byte_no], (const char *)result_ptr->SSID, ssid_len);
            byte_no = byte_no + ssid_len;

            /* Fill in the 4 bytes of security value */
            scan_data[byte_no++] = SCAN_PACKET_TYPE_SECURITY;
            scan_data[byte_no++] = sizeof(uint32_t);
            scan_data[byte_no++] = (uint8_t)(result_ptr->security & MASK_8BITS);
            scan_data[byte_no++] = (uint8_t)((result_ptr->security &
                                              MASK_16BITS) >> SHIFT_8BITS);
            scan_data[byte_no++] = (uint8_t)((result_ptr->security &
                                              MASK_24BITS) >> SHIFT_16BITS);
            scan_data[byte_no++] = (uint8_t)((result_ptr->security &
                                              MASK_32BITS) >> SHIFT_24BITS);
            if ((conn_id != 0) &&
                (app_custom_service_wifi_networks_client_char_config[0] &
                GATT_CLIENT_CONFIG_NOTIFICATION))
            {
                wiced_bt_gatt_server_send_notification(conn_id,
                        HDLC_CUSTOM_SERVICE_WIFI_NETWORKS_VALUE, byte_no, scan_data,
                    (wiced_bt_gatt_app_context_t)wiced_bt_free_buffer);

            }
        }
    }

    /* If the connect bit is set in the user data then connect notification to
     * the WiFi task
     */
    if ((CY_WCM_SCAN_COMPLETE == status) && ((NOTIF_SCAN | NOTIF_CONNECT) ==
                                              *(uint32_t *) user_data))
    {
        xTaskNotifyFromISR(wifi_task_handle, NOTIF_CONNECT,
                           eSetValueWithOverwrite, NULL);
    }
}


/* [] END OF FILE */

