/******************************************************************************
* File Name:   led_task.h
*
* Description: This file is the public interface of led_task.c
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

#ifndef SOURCE_LED_TASK_H_
#define SOURCE_LED_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cy_mqtt_api.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Priority and stack size of the LED task. cy_mqtt_subscribe() is forwarded
 * to CM33 through VCM, so this task does not run the MQTT stack itself.
 */
#define TASK_LED_PRIORITY           (configMAX_PRIORITIES - 3U)
#define TASK_LED_STACK_SIZE         (1024U * 2U)

/* 8-bit value denoting the LED state. */
#define ON_STATE                    (0x01U)
#define OFF_STATE                   (0x00U)

/*******************************************************************************
* Data structure and enumeration
*******************************************************************************/
/* Available LED commands */
typedef enum
{
    SUBSCRIBE_TO_TOPIC,
    UNSUBSCRIBE_FROM_TOPIC,
    UPDATE_DEVICE_STATE
} led_command_t;

/* Structure used for storing LED data */
typedef struct
{
    led_command_t command;
    uint8_t data;
} led_command_data_t;

/*******************************************************************************
* Extern variables
*******************************************************************************/
extern QueueHandle_t led_command_data_q;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void task_led(void *param);
void virtual_mqtt_subscription_callback(cy_mqtt_received_msg_info_t *received_msg_info);

#ifdef __cplusplus
}
#endif

#endif /* SOURCE_LED_TASK_H_ */

/* [] END OF FILE */
