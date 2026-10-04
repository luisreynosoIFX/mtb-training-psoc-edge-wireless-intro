/******************************************************************************
* File Name:   virtual_mqtt_task.h
*
* Description: This file is the public interface of virtual_mqtt_task.c
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

#ifndef SOURCE_VIRTUAL_MQTT_TASK_H_
#define SOURCE_VIRTUAL_MQTT_TASK_H_

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
/* Priority of the Virtual MQTT task. configMAX_PRIORITIES is defined in
 * the FreeRTOSConfig.h and higher priority numbers denote high priority tasks.
 */
#define TASK_VIRTUAL_PRIORITY       (configMAX_PRIORITIES - 1U)

/* Stack size of the Virtual MQTT task */
#define TASK_VIRTUAL_STACK_SIZE     (1024U * 2U)

/*******************************************************************************
* Data structure and enumeration
*******************************************************************************/
/* Commands for the Virtual MQTT Client Task. */
typedef enum
{
    HANDLE_VIRTUAL_MQTT_SUBSCRIBE_FAILURE,
    HANDLE_VIRTUAL_MQTT_PUBLISH_FAILURE,
    HANDLE_VIRTUAL_MQTT_DISCONNECTION
} virtual_mqtt_task_cmd_t;

/*******************************************************************************
* Extern variables
*******************************************************************************/
extern QueueHandle_t virtual_mqtt_task_data_q;
extern cy_mqtt_t virtual_mqtt_connection;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void virtual_mqtt_task(void *param);

#ifdef __cplusplus
}
#endif

#endif /* SOURCE_VIRTUAL_MQTT_TASK_H_ */

/* [] END OF FILE */
