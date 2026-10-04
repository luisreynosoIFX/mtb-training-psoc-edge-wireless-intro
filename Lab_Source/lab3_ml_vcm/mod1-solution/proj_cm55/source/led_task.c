/******************************************************************************
* File Name:   led_task.c
*
* Description: This file contains the task that subscribes to the topic
*              'SECONDARY_SUB_TOPIC' through the MQTT connection of CM33 and
*              actuates the user LED based on the received MQTT messages.
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
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Task header files */
#include "led_task.h"
#include "virtual_mqtt_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "safe_log.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3U)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000U)

/* The number of MQTT topics to be subscribed to. */
#define SUBSCRIPTION_COUNT                      (1U)

/* Subscriber topic used for secondary core (CM55) */
#define SECONDARY_SUB_TOPIC                     "RED_APP_STATUS"

#define NO_RETRIES                              (0U)
#define COMPARE_SUCCESS                         (0)

#if defined(CYBSP_LED_RGB_GREEN_PORT) && defined(CYBSP_LED_RGB_GREEN_PIN)
#define APP_GREEN_LED_PORT CYBSP_LED_RGB_GREEN_PORT
#define APP_GREEN_LED_PIN  CYBSP_LED_RGB_GREEN_PIN
#elif defined(CYBSP_LED_GREEN_PORT) && defined(CYBSP_LED_GREEN_PIN)
#define APP_GREEN_LED_PORT CYBSP_LED_GREEN_PORT
#define APP_GREEN_LED_PIN  CYBSP_LED_GREEN_PIN
#else
#error "Selected BSP does not define a supported green LED"
#endif

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Queue handle used for LED data */
QueueHandle_t led_command_data_q;

/* Configure the subscription information structure for secondary core. */
static cy_mqtt_subscribe_info_t secondary_subscriber_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = SECONDARY_SUB_TOPIC,
    .topic_len = (sizeof(SECONDARY_SUB_TOPIC) - 1)
};

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void subscribe_to_topic(void);
static void unsubscribe_from_topic(void);

/*******************************************************************************
* Function Name: task_led
********************************************************************************
* Summary:
*  Task that subscribes to the specified MQTT topic and controls the user LED
*  based on the received commands over the message queue. The task can also
*  unsubscribe from the topic based on the commands via the message queue.
*
* Parameters:
*  void *param : Task parameter defined during task creation (unused)
*
* Return:
*  void
*******************************************************************************/
void task_led(void *param)
{
    led_command_data_t led_cmd_data;

    /* Suppress warning for unused parameter */
    (void) param;

    /* Subscribe to the specified MQTT topic. */
    subscribe_to_topic();

    /* Repeatedly running part of the task */
    for (;;)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(led_command_data_q, &led_cmd_data, portMAX_DELAY))
        {
            switch(led_cmd_data.command)
            {
                case SUBSCRIBE_TO_TOPIC:
                {
                    subscribe_to_topic();
                    break;
                }

                case UNSUBSCRIBE_FROM_TOPIC:
                {
                    unsubscribe_from_topic();
                    break;
                }

                case UPDATE_DEVICE_STATE:
                {
                    /* Update the LED state as per received notification. */
                    Cy_GPIO_Write(APP_GREEN_LED_PORT, APP_GREEN_LED_PIN,
                                  led_cmd_data.data);
                    break;
                }

                default:
                    break;
            }
        }
    }
}

/*******************************************************************************
* Function Name: subscribe_to_topic
********************************************************************************
* Summary:
*  Function that subscribes to the MQTT topic specified by the macro
*  'SECONDARY_SUB_TOPIC'. This operation is retried a maximum of
*  'MAX_SUBSCRIBE_RETRIES' times with interval of
*  'MQTT_SUBSCRIBE_RETRY_INTERVAL_MS' milliseconds.
*
* Parameters:
*  void
*
* Return:
*  void
*******************************************************************************/
static void subscribe_to_topic(void)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Command to the Virtual MQTT task */
    virtual_mqtt_task_cmd_t virtual_mqtt_task_cmd;

    /* Subscribe with the configured parameters. */
    for (uint32_t retry_count = NO_RETRIES; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++)
    {
        result = cy_mqtt_subscribe(virtual_mqtt_connection, &secondary_subscriber_info,
                                   SUBSCRIPTION_COUNT);
        if (CY_RSLT_SUCCESS == result)
        {
            safe_print("\nMQTT client subscribed to the topic '%.*s' successfully.\r\n",
                       secondary_subscriber_info.topic_len, secondary_subscriber_info.topic);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
    }

    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("\nMQTT Subscribe failed with error 0x%0X after %d retries...\r\n",
                   (int)result, MAX_SUBSCRIBE_RETRIES);

        /* Notify the Virtual MQTT task about the subscription failure */
        virtual_mqtt_task_cmd = HANDLE_VIRTUAL_MQTT_SUBSCRIBE_FAILURE;
        xQueueSend(virtual_mqtt_task_data_q, &virtual_mqtt_task_cmd, portMAX_DELAY);
    }
}

/*******************************************************************************
* Function Name: virtual_mqtt_subscription_callback
********************************************************************************
* Summary:
*  Callback to handle incoming MQTT messages. This callback prints the
*  contents of the incoming message and informs the LED task, via a message
*  queue, to turn on / turn off the LED based on the received message.
*
* Parameters:
*  cy_mqtt_received_msg_info_t *received_msg_info : Information structure of
*                                                   the received MQTT message
*
* Return:
*  void
*******************************************************************************/
void virtual_mqtt_subscription_callback(cy_mqtt_received_msg_info_t *received_msg_info)
{
    /* Received MQTT message */
    const char *received_msg = received_msg_info->payload;
    size_t received_msg_len = received_msg_info->payload_len;

    /* Data to be sent to the LED task queue. */
    led_command_data_t led_cmd_data;

    /* The MQTT handle is shared across both the cores, so both the cores
     * receive all subscription messages. Ignore the topics of the other core.
     */
    if ((received_msg_info->topic_len != secondary_subscriber_info.topic_len) ||
        (COMPARE_SUCCESS != strncmp(SECONDARY_SUB_TOPIC, received_msg_info->topic,
                                    received_msg_info->topic_len)))
    {
        return;
    }

    safe_print("\nSubscriber(m55): Incoming MQTT message received:\r\n"
               "    Received topic name: %.*s\r\n"
               "    Received QoS: %d\r\n"
               "    Received payload: %.*s\r\n",
               received_msg_info->topic_len, received_msg_info->topic,
               (int) received_msg_info->qos,
               (int) received_msg_len, received_msg);

    /* Assign the LED state depending on the received MQTT message. */
    if ((strlen(ON_MESSAGE) == received_msg_len) &&
        (COMPARE_SUCCESS == strncmp(ON_MESSAGE, received_msg, received_msg_len)))
    {
        led_cmd_data.data = ON_STATE;
    }
    else if ((strlen(OFF_MESSAGE) == received_msg_len) &&
             (COMPARE_SUCCESS == strncmp(OFF_MESSAGE, received_msg, received_msg_len)))
    {
        led_cmd_data.data = OFF_STATE;
    }
    else
    {
        safe_print("  Subscriber: Received MQTT message not in valid format!\r\n");
        return;
    }

    /* Send the command and data to LED task queue */
    led_cmd_data.command = UPDATE_DEVICE_STATE;
    xQueueSend(led_command_data_q, &led_cmd_data, portMAX_DELAY);
}

/*******************************************************************************
* Function Name: unsubscribe_from_topic
********************************************************************************
* Summary:
*  Function that unsubscribes from the topic specified by the macro
*  'SECONDARY_SUB_TOPIC'.
*
* Parameters:
*  void
*
* Return:
*  void
*******************************************************************************/
static void unsubscribe_from_topic(void)
{
    cy_rslt_t result = cy_mqtt_unsubscribe(virtual_mqtt_connection,
                                           (cy_mqtt_unsubscribe_info_t *) &secondary_subscriber_info,
                                           SUBSCRIPTION_COUNT);

    if (CY_RSLT_SUCCESS != result)
    {
        safe_print("MQTT Unsubscribe operation failed with error 0x%0X!\r\n", (int)result);
    }
}

/* [] END OF FILE */
