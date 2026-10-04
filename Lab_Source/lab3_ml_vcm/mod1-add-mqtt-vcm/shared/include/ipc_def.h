/******************************************************************************
* File Name:   ipc_def.h
*
* Description:
*  Auxiliary header with constants and structures for the IPC.
*
* Related Document: See README.md
*
*
*******************************************************************************
 * (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
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

#ifndef IPC_DEF_H
#define IPC_DEF_H

    /* Enable/Disable the usage of IPC Semaphore */
    #define ENABLE_SEMA             (1U)

    #define IPC_CHANNEL_NUM         (8U)

    /* Semaphore number to be used in this example. Semaphores 0-15 are reserved
       for system use. */
    #define MAX_SEMA_NUM            (32U)

    #define SEMA_NUM                (16U)
    
    /* Set by CM33 once the MQTT connection is ready to be shared with CM55 */
    #define MQTT_READY_SEMA_NUM     (17U)

    /* Delay to allow other core to acquire the semaphore */
    #define SEMA_DELAY              (10U)

#endif /* IPC_DEF_H */

/* [] END OF FILE */