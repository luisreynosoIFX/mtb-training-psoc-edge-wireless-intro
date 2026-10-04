/******************************************************************************
* File Name:   safe_log.c
*
* Description: This file provides a lightweight, IPC-safe logging utility
*              for dual-core PSoC™ Edge applications. It implements the
*              `safe_print()` function used for sending formatted debug
*              messages over UART without causing output corruption between
*              CM33 and CM55 cores.
*
*              It demonstrates:
*              - Formatting printf-style strings using a shared log buffer
*              - Protecting UART access using an IPC semaphore
*              - Providing a centralized, thread-safe print mechanism
*                for all application modules
*
* Related Document: See README.md
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

#include "safe_log.h"
#include <stdio.h>
#include <stdarg.h>
#include "ipc_def.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"

#define LOG_BUFFER_SIZE 256
static char log_buffer[LOG_BUFFER_SIZE];

/*******************************************************************************
* Function Name: safe_print
********************************************************************************
*
* Summary:
*  Prints a formatted debug message over UART using an IPC-protected mechanism
*  to avoid output corruption between the CM33 and CM55 cores. This function
*  formats the input string, acquires the IPC semaphore for UART access,
*  transmits the message, and releases the semaphore.
*
* Parameters:
*  const char *fmt : Pointer to a printf-style format string
*  ...             : Additional arguments referenced by the format string
*
* Return:
*  void
*
*******************************************************************************/
void safe_print(const char *fmt, ...)
{
    va_list args;

    /* Taken before formatting: the lock also guards log_buffer between tasks on this core. */
    while (Cy_IPC_Sema_Set(SEMA_NUM, false) != CY_IPC_SEMA_SUCCESS)
    {
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            vTaskDelay(1U);
        }
    }

    va_start(args, fmt);
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);
    va_end(args);

    /* UART TX */
    for (size_t index = 0; log_buffer[index] != '\0'; index++)
    {
        if (log_buffer[index] == '\n' &&
            (index == 0 || log_buffer[index - 1] != '\r'))
        {
            while (Cy_SCB_UART_Put(CYBSP_DEBUG_UART_HW, '\r') == 0U) {}
        }
        while (Cy_SCB_UART_Put(CYBSP_DEBUG_UART_HW,
                              (uint32_t)(unsigned char)log_buffer[index]) == 0U) {}
    }

    /* Unlock */
    Cy_IPC_Sema_Clear(SEMA_NUM, false);
}
