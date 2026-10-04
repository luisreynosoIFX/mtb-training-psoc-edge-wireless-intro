/******************************************************************************
* File Name:   virtual_mqtt_task.c
*
* Description: This file contains the task that initializes the Virtual
*              Connectivity Manager (VCM) on CM55, gets the MQTT connection
*              created by CM33, and starts the publisher and LED tasks.
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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ipc_def.h"

/* Task header files */
#include "virtual_mqtt_task.h"
#include "led_task.h"
#include "publisher_task.h"

/* Middleware libraries */
#include "cy_wcm.h"
#include "cy_vcm.h"
#include "cy_mqtt_api.h"
#include "safe_log.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Descriptor of the MQTT connection created by CM33. */
#define MQTT_HANDLE_DESCRIPTOR           "MQTThandleID"

#define WAIT_DELAY_MS                    (2000U)

/* Queue lengths of message queues used in this project */
#define SINGLE_ELEMENT_QUEUE             (1U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT connection handle shared by CM33 */
cy_mqtt_t virtual_mqtt_connection;

/* Queue handle used for Virtual MQTT task commands */
QueueHandle_t virtual_mqtt_task_data_q;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void vcm_callback(cy_vcm_event_t event);
static void wcm_event_callback(cy_wcm_event_t event, cy_wcm_event_data_t *event_data);
static void virtual_mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event,
                                        void *user_data);

/*******************************************************************************
* Function Name: virtual_mqtt_task
********************************************************************************
* Summary:
*  Task that initializes VCM on CM55, waits until CM33 has connected to the
*  MQTT broker, gets the MQTT connection handle, and creates the publisher
*  and LED tasks. It then handles the failures reported by those tasks.
*
* Parameters:
*  void *arg : Task parameter defined during task creation (unused)
*
* Return:
*  void
*******************************************************************************/
void virtual_mqtt_task(void *arg)
{
    cy_rslt_t result;
    cy_vcm_config_t secondary_core_config;
    virtual_mqtt_task_cmd_t virtual_mqtt_task_cmd;
    led_command_data_t led_cmd_data;

    (void) arg;

    /* TODO 2: Initialize VCM on CM55. CM33 created the IPC resources before
     * booting CM55, so CM55 uses them.
     *  - Fill in secondary_core_config: ipc_obj = &cybsp_cm55_ipc_instance,
     *    hal_resource_opt = CY_VCM_USE_HAL_RESOURCE,
     *    channel_num = MTB_IPC_CHAN_1, event_cb = vcm_callback.
     *  - Call cy_vcm_init() and store the return value in 'result' (replace
     *    the placeholder below).
     */
    result = ~CY_RSLT_SUCCESS;

    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("\nVCM Initialization failed on CM55.\r\n");
        vTaskDelete(NULL);
    }
    safe_print("\nVirtual Connectivity Manager Initialized on CM55\r\n");

    /* TODO 3: Wait until CM33 signals that the MQTT connection is ready:
     * while Cy_IPC_Sema_Status(MQTT_READY_SEMA_NUM) returns
     * CY_IPC_SEMA_STATUS_UNLOCKED, call vTaskDelay(pdMS_TO_TICKS(WAIT_DELAY_MS)).
     */
    safe_print("\nPlease wait until MQTT is connected on CM33...\r\n");

    /* Create the queues. See the respective data-types for queue contents */
    led_command_data_q = xQueueCreate(SINGLE_ELEMENT_QUEUE, sizeof(led_command_data_t));
    virtual_mqtt_task_data_q = xQueueCreate(SINGLE_ELEMENT_QUEUE, sizeof(virtual_mqtt_task_cmd_t));

    /* Wi-Fi and MQTT initialization. These calls are forwarded to CM33. */
    result = cy_wcm_init(NULL);
    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("\nWi-Fi Connection Manager Initialization failed on CM55.\r\n");
    }

    result = cy_mqtt_init();
    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("\nMQTT Initialization failed on CM55.\r\n");
        vTaskDelete(NULL);
    }

    safe_print("\nWi-Fi connection status : %s\r\n",
               (1 == cy_wcm_is_connected_to_ap()) ? "Connected" : "Disconnected");

    result = cy_wcm_register_event_callback(wcm_event_callback);
    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("Wi-Fi event callback could not be registered\r\n");
    }

    /* Get the MQTT connection created by CM33. */
    result = cy_mqtt_get_handle(&virtual_mqtt_connection, MQTT_HANDLE_DESCRIPTOR);
    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("\nMQTT connection could not be obtained on CM55 (0x%0X)\r\n", (int)result);
        vTaskDelete(NULL);
    }
    safe_print("\nMQTT connection obtained on CM55\r\n");

    result = cy_mqtt_register_event_callback(virtual_mqtt_connection,
                                             (cy_mqtt_callback_t)virtual_mqtt_event_callback, NULL);
    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("MQTT event callback could not be registered\r\n");
        vTaskDelete(NULL);
    }

    /* Create the user tasks. See the respective task definition for more
     * details of these tasks.
     */
    xTaskCreate(publisher_task, "Publisher task", PUBLISHER_TASK_STACK_SIZE,
                NULL, PUBLISHER_TASK_PRIORITY, &publisher_task_handle);
    xTaskCreate(task_led, "LED task", TASK_LED_STACK_SIZE,
                NULL, TASK_LED_PRIORITY, NULL);

    /* Repeatedly running part of the task */
    for (;;)
    {
        /* Block until a command has been received over queue */
        if (pdTRUE == xQueueReceive(virtual_mqtt_task_data_q, &virtual_mqtt_task_cmd,
                                    portMAX_DELAY))
        {
            switch(virtual_mqtt_task_cmd)
            {
                case HANDLE_VIRTUAL_MQTT_PUBLISH_FAILURE:
                {
                    /* Handle Publish Failure here. */
                    break;
                }

                case HANDLE_VIRTUAL_MQTT_SUBSCRIBE_FAILURE:
                {
                    /* Re-initiate MQTT subscribe after a delay. */
                    vTaskDelay(pdMS_TO_TICKS(WAIT_DELAY_MS));
                    led_cmd_data.command = SUBSCRIBE_TO_TOPIC;
                    xQueueSend(led_command_data_q, &led_cmd_data, portMAX_DELAY);
                    break;
                }

                case HANDLE_VIRTUAL_MQTT_DISCONNECTION:
                {
                    /* CM33 reconnects to the MQTT broker. Subscribe again once
                     * the Wi-Fi connection is back.
                     */
                    if (1 == cy_wcm_is_connected_to_ap())
                    {
                        safe_print("\nWi-Fi connection is alive\r\n");
                        led_cmd_data.command = SUBSCRIBE_TO_TOPIC;
                        xQueueSend(led_command_data_q, &led_cmd_data, portMAX_DELAY);
                    }
                    else
                    {
                        safe_print("\nWaiting for Wi-Fi connection to be back alive\r\n");
                        vTaskDelay(pdMS_TO_TICKS(WAIT_DELAY_MS));
                        virtual_mqtt_task_cmd = HANDLE_VIRTUAL_MQTT_DISCONNECTION;
                        xQueueSend(virtual_mqtt_task_data_q, &virtual_mqtt_task_cmd,
                                   portMAX_DELAY);
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
}

/*******************************************************************************
* Function Name: virtual_mqtt_event_callback
********************************************************************************
* Summary:
*  Callback invoked by the MQTT library for events like MQTT disconnection,
*  incoming MQTT subscription messages from the MQTT broker.
*    1. In case of MQTT disconnection, the Virtual MQTT task is communicated
*       about the disconnection using a message queue.
*    2. When an MQTT subscription message is received, the subscriber callback
*       function implemented in led_task.c is invoked to handle the incoming
*       MQTT message.
*
* Parameters:
*  cy_mqtt_t mqtt_handle : MQTT handle corresponding to the MQTT event (unused)
*  cy_mqtt_event_t event : MQTT event information
*  void *user_data : User data pointer passed during callback registration (unused)
*
* Return:
*  void
*******************************************************************************/
static void virtual_mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event,
                                        void *user_data)
{
    virtual_mqtt_task_cmd_t virtual_mqtt_task_cmd;

    (void) mqtt_handle;
    (void) user_data;

    switch(event.type)
    {
        case CY_MQTT_EVENT_TYPE_DISCONNECT:
        {
            /* MQTT connection with the MQTT broker is broken. */
            safe_print("\nUnexpectedly disconnected from MQTT broker!\r\n");
            virtual_mqtt_task_cmd = HANDLE_VIRTUAL_MQTT_DISCONNECTION;
            xQueueSend(virtual_mqtt_task_data_q, &virtual_mqtt_task_cmd, portMAX_DELAY);
            break;
        }

        case CY_MQTT_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECEIVE:
        {
            /* Incoming MQTT message has been received. Send this message to
             * the subscriber callback function to handle it.
             */
            virtual_mqtt_subscription_callback(&(event.data.pub_msg.received_message));
            break;
        }

        default:
        {
            /* Unknown MQTT event */
            safe_print("\nUnknown Event received from MQTT callback!\r\n");
            break;
        }
    }
}

/*******************************************************************************
* Function Name: wcm_event_callback
********************************************************************************
* Summary:
*  Callback invoked by the WCM library for events like Wi-Fi disconnection,
*  connection, reconnection or change in IP, etc.
*
* Parameters:
*  cy_wcm_event_t event : WCM event information
*  cy_wcm_event_data_t *event_data : WCM event data (unused)
*
* Return:
*  void
*******************************************************************************/
static void wcm_event_callback(cy_wcm_event_t event, cy_wcm_event_data_t *event_data)
{
    (void) event_data;

    if (CY_WCM_EVENT_DISCONNECTED == event)
    {
        safe_print("\nWi-Fi disconnected (reported to CM55)\r\n");
    }
}

/*******************************************************************************
* Function Name: vcm_callback
********************************************************************************
* Summary:
*  Callback invoked by VCM library for events like Virtualization init or
*  deinit. The VCM library requires it even though CM55 does not receive the
*  INIT_COMPLETE event (it is sent to CM33, which initialized VCM first).
*
* Parameters:
*  cy_vcm_event_t event : VCM event information
*
* Return:
*  void
*******************************************************************************/
static void vcm_callback(cy_vcm_event_t event)
{
    if (CY_VCM_EVENT_DEINIT == event)
    {
        safe_print("\nVCM Callback: received DEINIT from CM33\r\n");
    }
}

/* [] END OF FILE */
