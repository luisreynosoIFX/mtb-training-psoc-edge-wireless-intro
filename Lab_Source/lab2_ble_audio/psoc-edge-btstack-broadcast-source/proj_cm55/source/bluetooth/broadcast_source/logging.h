/******************************************************************************
* File Name        : logging.h
*
* Description      : This file defines application logging macros.
*
* Related Document : See README.md
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
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

#ifndef __LOG_H__
#define __LOG_H__

/******************************************************************************
* Header Files
*******************************************************************************/
#include <stdio.h>

/******************************************************************************
* Macros
*******************************************************************************/
/* Enable the below macro switch to enable trace logging */
#define ENABLE_WICED_TRACE_LOGS     (0)

#define TAG ""

#define TRACE_MSG(f_, ...) printf((f_), ##__VA_ARGS__), \
                           printf("\n")

#define TRACE_LOG(f_, ...) do { \
    if (ENABLE_WICED_TRACE_LOGS) { \
        printf("%s[%s]:", TAG, __func__); \
        printf((f_), ##__VA_ARGS__); \
        printf("\n"); \
    } \
} while (0)

#define TRACE_ERR(f_, ...) printf("%s[ERROR]", TAG), \
                           printf("[%s]:", __func__), \
                           printf((f_), ##__VA_ARGS__), \
                           printf("\n")

#ifdef WICED_BT_TRACE
    #undef WICED_BT_TRACE
#endif /* WICED_BT_TRACE */

#if ENABLE_WICED_TRACE_LOGS
#define WICED_BT_TRACE     printf
#else
#define WICED_BT_TRACE(...)
#endif /* ENABLE_WICED_TRACE_LOGS */

#endif

/* [] END OF FILE */
