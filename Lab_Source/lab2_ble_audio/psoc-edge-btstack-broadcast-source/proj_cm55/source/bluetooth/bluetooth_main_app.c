/******************************************************************************
* File Name        : bluetooth_main_app.c
*
* Description      : This file contains function definitions for setting up the
*                    Bluetooth interface and starting the main BT task for the
*                    Broadcast Source application.
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

/******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"

#include "bluetooth_main_app.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_stack.h"
#include "app_bt_utils.h"
#include "broadcast_source_bt_manager.h"

/******************************************************************************
* Macros
*******************************************************************************/
#define MAX_PATH                        ( 256u )


/******************************************************************************
* Global Variables
*******************************************************************************/
TaskHandle_t bt_task_handle;
wiced_bt_heap_t *p_default_heap = NULL;


/******************************************************************************
* Function Prototypes
*******************************************************************************/
uint32_t hci_control_proc_rx_cmd(uint8_t *p_buffer, uint32_t length);
static void application_start(void *pvParameters);


/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: hci_control_proc_rx_cmd
*******************************************************************************
* Summary:
*   Function to handle HCI RX command.
*
* Parameters:
*   p_buffer : Pointer to the RX buffer
*   length   : Size of the RX buffer
*
* Return:
*   uint32_t - Always returns 0.
*
*******************************************************************************/
uint32_t hci_control_proc_rx_cmd(uint8_t *p_buffer, uint32_t length)
{
    return 0;
}

/******************************************************************************
* Function Name: application_start
*******************************************************************************
* Summary:
*   Task that initializes the BT stack and creates the heap for Bluetooth
*   operations. For the Broadcast Source, no bonding or key storage is
*   required as the device only broadcasts and has no LE connections.
*
* Parameters:
*   pvParameters : Task parameter defined during task creation (unused)
*
* Return:
*   None
*
*******************************************************************************/
static void application_start(void *pvParameters)
{
    wiced_result_t wiced_result = WICED_BT_SUCCESS;

    vTaskDelay(pdMS_TO_TICKS(BT_TASK_START_DELAY_MS));

    /* Register callback and configuration with BT stack */
    printf("Initializing BT Stack...\n");
    wiced_result = wiced_bt_stack_init(broadcast_source_btm_cback,
                                       &broadcast_source_cfg_settings);

    /* Check if stack initialization was successful */
    if (WICED_BT_SUCCESS == wiced_result)
    {
        /* Create a buffer heap, make it the default heap.  */
        printf("Creating heap...\n");
        p_default_heap = wiced_bt_create_heap("app", NULL, BT_STACK_HEAP_SIZE,
                                              NULL, WICED_TRUE);
    }

    if ((WICED_BT_SUCCESS == wiced_result) && (NULL != p_default_heap))
    {
        printf("Bluetooth Host Stack Initialization Successful!\n");
    }
    else
    {
        /* Exit App if stack init was not successful or heap creation failed */
        printf(">> Error: Bluetooth Host Stack Initialization or heap creation failed!!\n");
        printf("   Halting the application...\n");
        CY_ASSERT(0);
    }

    vTaskDelete(NULL);
}

/******************************************************************************
* Function Name: bt_app_init
*******************************************************************************
* Summary:
*   Function to setup the Bluetooth platform and create the main BT task.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void bt_app_init(void)
{
    BaseType_t status;

    /* Create the main BT task. */
    status = xTaskCreate(application_start, "BT task", BT_TASK_STACK_SIZE, NULL,
                         BT_TASK_PRIORITY, &bt_task_handle);
    if (status != pdPASS)
    {
        printf(">> Error in starting BT task \n");
        CY_ASSERT(0);
    }
}

/* [] END OF FILE */
