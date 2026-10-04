/******************************************************************************
* File Name:   publisher_task.c
*
* Description: This file contains the task that publishes the device state
*              "ON"/"OFF" on the topic 'SECONDARY_PUB_TOPIC' each time the
*              user button (SW1) is pressed. CM55 publishes through the MQTT
*              connection of CM33 using the Virtual Connectivity Manager.
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

/* Task header files */
#include "publisher_task.h"
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
/* Queue length of a message queue that is used to communicate with the
 * publisher task.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH     (3U)

/* Publisher topic used for secondary core (CM55). CM33 subscribes to it. */
#define SECONDARY_PUB_TOPIC             "GREEN_APP_STATUS"
#define INFERENCE_RESULT_TOPIC          "ML_INFERENCE_RESULT"

/* Button presses closer than this are treated as switch bounce. */
#define DEBOUNCE_TIME_MS                (200U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT publish settings for inference status updates. */
static cy_mqtt_publish_info_t inference_publish_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = INFERENCE_RESULT_TOPIC,
    .topic_len = (sizeof(INFERENCE_RESULT_TOPIC) - 1),
    .retain = false,
    .dup = false
};

/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Handle of the queue holding the commands for the publisher task */
QueueHandle_t publisher_task_q;

/* Device state that is toggled each time the user button is pressed. */
static uint32_t current_device_state = OFF_STATE;

/* Tick count of the last accepted button press. */
static TickType_t last_button_press_time;

/* Structure to store publish message information. */
static cy_mqtt_publish_info_t secondary_publish_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = SECONDARY_PUB_TOPIC,
    .topic_len = (sizeof(SECONDARY_PUB_TOPIC) - 1),
    .retain = false,
    .dup = false
};

/* Queue a changed inference status for the publisher task. */
BaseType_t publisher_publish_inference_result(const char *status)
{
    publisher_data_t publisher_q_data;

    if ((NULL == publisher_task_q) || (NULL == status))
    {
        return pdFALSE;
    }

    publisher_q_data.cmd = PUBLISH_INFERENCE_RESULT;
    publisher_q_data.data = (char *)status;
    return xQueueSend(publisher_task_q, &publisher_q_data, 0U);
}
/*******************************************************************************
* Function Name: publisher_task
********************************************************************************
* Summary:
*  Task that publishes MQTT messages to the broker. The messages are sent to
*  this task over a message queue by the user button interrupt handler.
*
* Parameters:
*  void *pvParameters : Task parameter defined during task creation (unused)
*
* Return:
*  void
*******************************************************************************/
void publisher_task(void *pvParameters)
{
    /* Status variable */
    cy_rslt_t result;

    publisher_data_t publisher_q_data;

    /* Command to the Virtual MQTT task */
    virtual_mqtt_task_cmd_t virtual_mqtt_task_cmd;
    cy_mqtt_publish_info_t *publish_info;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Create a message queue to communicate with the button interrupt. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

    safe_print("\nPress the user button (SW1) to publish \"%s\"/\"%s\" on the topic '%s'...\r\n",
               ON_MESSAGE, OFF_MESSAGE, secondary_publish_info.topic);

    while (true)
    {
        /* Wait for commands from the button interrupt. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data.cmd)
            {
                case PUBLISH_MQTT_MSG:
                {
                    publish_info = &secondary_publish_info;
                    break;
                }

                case PUBLISH_INFERENCE_RESULT:
                {
                    publish_info = &inference_publish_info;
                    break;
                }

                default:
                    continue;
            }

            publish_info->payload = publisher_q_data.data;
            publish_info->payload_len = strlen(publish_info->payload);

            safe_print("\nPublisher(m55): Publishing '%s' on the topic '%s'\r\n",
                       (char *)publish_info->payload, publish_info->topic);

            result = cy_mqtt_publish(virtual_mqtt_connection, publish_info);
            if (CY_RSLT_SUCCESS != result)
            {
                safe_print("  Publisher: MQTT Publish failed with error 0x%0X.\r\n",
                           (int)result);

                /* Communicate the publish failure with the Virtual MQTT task. */
                virtual_mqtt_task_cmd = HANDLE_VIRTUAL_MQTT_PUBLISH_FAILURE;
                xQueueSend(virtual_mqtt_task_data_q, &virtual_mqtt_task_cmd, portMAX_DELAY);
            }
        }
    }
}

/*******************************************************************************
* Function Name: publisher_button_interrupt_handler
********************************************************************************
* Summary:
*  User button (SW1) interrupt handler. SW1 shares the GPIO port interrupt with
*  the IMU, so this function is called from the IMU interrupt handler in imu.c.
*  It detects button presses and sends the publish command along with the data
*  to be published to the publisher task over a message queue. Based on the
*  current device state, the publish data is set so that the device state gets
*  toggled.
*
* Parameters:
*  void
*
* Return:
*  void
*******************************************************************************/
void publisher_button_interrupt_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    publisher_data_t publisher_q_data;
    TickType_t now;

    /* Check if the interrupt was from the user button */
    if (0U == Cy_GPIO_GetInterruptStatusMasked(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN))
    {
        return;
    }

    /* Clear the interrupt */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);

    /* Ignore presses until the publisher task is ready, and switch bounce. */
    now = xTaskGetTickCountFromISR();
    if ((NULL == publisher_task_q) ||
        ((now - last_button_press_time) < pdMS_TO_TICKS(DEBOUNCE_TIME_MS)))
    {
        return;
    }
    last_button_press_time = now;

    /* Toggle the device state and publish it */
    publisher_q_data.cmd = PUBLISH_MQTT_MSG;
    if (ON_STATE == current_device_state)
    {
        current_device_state = OFF_STATE;
        publisher_q_data.data = (char *) OFF_MESSAGE;
    }
    else
    {
        current_device_state = ON_STATE;
        publisher_q_data.data = (char *) ON_MESSAGE;
    }

    xQueueSendFromISR(publisher_task_q, &publisher_q_data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* [] END OF FILE */
