/******************************************************************************
* File Name        : btn_mgmt.c
*
* Description      : This file contains the source code for the user button
*                    management in the Broadcast Sink application.
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
#include "btn_mgmt.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "cy_retarget_io.h"
#include "wiced_bt_trace.h"


/******************************************************************************
* Macros
*******************************************************************************/
#define BTN_TIMER_MULTIPLIER            (10u)
#define BTN_TIMER_FREQUENCY_HZ          (1000UL * BTN_TIMER_MULTIPLIER)
#define BTN_TIMER_INTERRUPT_PRIORITY    (3u)
#define BTN_GPIO_INTERRUPT_PRIORITY     (2u)
#define PORT_INTR_MASK                  (0x00000001UL << 8)

/******************************************************************************
* Data structures and enumeration
*******************************************************************************/
/* Data-type for user button number */
typedef enum
{
    USER_BTN1 = 1u,
    USER_BTN2,
    USER_BTN_INVALID,
} btn_num_t;

/* Data-type for user button states */
typedef enum
{
    BTN_NOT_PRESSED = 1u,
    BTN_PRESSED,
    BTN_LONG_PRESS,
} btn_mgmt_states_t;

/* Data-type for Button Management task commands */
typedef enum
{
    BTN1_HANDLE_SHORT_PRESS_EVENT = 1u,
    BTN1_HANDLE_LONG_PRESS_EVENT,
    BTN2_HANDLE_SHORT_PRESS_EVENT,
    BTN2_HANDLE_LONG_PRESS_EVENT,
    BTN_CMD_INVALID,
} btn_mgmt_cmd_t;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static cy_rslt_t create_btn_mgmt_task(void);
static void btn_mgmt_task(void* pvParameters);
static void btn_gpio_interrupt_handler(void);
static void btn_debounce_timer_callback(TimerHandle_t xTimer);
static void btn_long_press_timer_callback(TimerHandle_t xTimer);
static inline void btn_handle_short_press(btn_num_t btn_num);
static inline void btn_handle_long_press(btn_num_t btn_num);

/******************************************************************************
* Global Variables
*******************************************************************************/
static TimerHandle_t btn_debounce_timer;
static TimerHandle_t btn_long_press_timer;
static btn_num_t active_btn_num = USER_BTN_INVALID;
static btn_mgmt_states_t active_btn_state = BTN_NOT_PRESSED;

/* Button Management task handle */
TaskHandle_t btn_mgmt_task_handle;

/* Handle of the queue holding the Button Management task commands. */
QueueHandle_t btn_mgmt_task_q;

/* Interrupt configuration structures for both button GPIOs */
static cy_stc_sysint_t btn1_intr_cfg =
{
    .intrSrc = CYBSP_USER_BTN1_IRQ,
    .intrPriority = BTN_GPIO_INTERRUPT_PRIORITY
};

static cy_stc_sysint_t btn2_intr_cfg =
{
    .intrSrc = CYBSP_USER_BTN2_IRQ,
    .intrPriority = BTN_GPIO_INTERRUPT_PRIORITY
};

/******************************************************************************
* Function Definitions
*******************************************************************************/

/******************************************************************************
* Function Name: btn_mgmt_init
*******************************************************************************
* Summary:
*   Function that initializes the GPIO pins, timers, and interrupt handlers
*   for both the user buttons and sets up the button management RTOS task.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void btn_mgmt_init(void)
{
    cy_rslt_t result;

    /* Clear GPIO and NVIC interrupt before initializing to avoid false
     * triggering.
     */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

    /* Initialize the interrupt and register interrupt callback */
    Cy_SysInt_Init(&btn1_intr_cfg, &btn_gpio_interrupt_handler);
    Cy_SysInt_Init(&btn2_intr_cfg, &btn_gpio_interrupt_handler);


    /* Create the FreeRTOS timers for debouncing and long press detection */
    btn_debounce_timer = xTimerCreate("Debounce Timer",
                                      pdMS_TO_TICKS(BTN_DEBOUNCE_INTERVAL_MS),
                                      pdFALSE,
                                      (void *) 0,
                                      btn_debounce_timer_callback);
    CY_ASSERT(btn_debounce_timer != NULL);

    btn_long_press_timer = xTimerCreate("Long Press Timer",
                                        pdMS_TO_TICKS(BTN_LONG_PRESS_INTERVAL_MS),
                                        pdFALSE,
                                        (void *) 0,
                                        btn_long_press_timer_callback);
    CY_ASSERT(btn_long_press_timer != NULL);

    /* Set-up Button Management Task and halt if failed. */
    result = create_btn_mgmt_task();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Suppress compiler warnings */
    CY_UNUSED_PARAMETER(result);

    /* Enable the interrupts in the NVIC */
    NVIC_EnableIRQ(btn1_intr_cfg.intrSrc);
    NVIC_EnableIRQ(btn2_intr_cfg.intrSrc);
}

/******************************************************************************
* Function Name: create_btn_mgmt_task
*******************************************************************************
* Summary:
*   Function that creates the User Button Management RTOS task.
*
* Parameters:
*   None
*
* Return:
*   CY_RSLT_SUCCESS upon successful creation of the task, else a non-zero value
*   that indicates the error.
*
*******************************************************************************/
static cy_rslt_t create_btn_mgmt_task(void)
{
    BaseType_t status;

    status = xTaskCreate(btn_mgmt_task, "Btn Mgmt Task",
                         BTN_MGMT_TASK_STACK_SIZE, NULL,
                         BTN_MGMT_TASK_PRIORITY, &btn_mgmt_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t) status;
}

/******************************************************************************
* Function Name: btn_gpio_interrupt_handler
*******************************************************************************
* Summary:
*   GPIO interrupt handler for User Buttons.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
static void btn_gpio_interrupt_handler(void)
{
    /* Get interrupt cause */
    uint32_t interrupt_cause = Cy_GPIO_GetInterruptCause0();

    /* Check if the interrupt was from the user button's port */
    if(PORT_INTR_MASK == (interrupt_cause & PORT_INTR_MASK))
    {
        if (1UL == Cy_GPIO_GetInterruptStatusMasked(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN))
        {
            /* Clear the interrupt */
            Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
            NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);

            active_btn_num = USER_BTN1;
        }

        if (1UL == Cy_GPIO_GetInterruptStatusMasked(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN))
        {
            /* Clear the interrupt */
            Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
            NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

            active_btn_num = USER_BTN2;
        }
    }

    /* Disable the GPIO pin interrupts for both buttons. */
    NVIC_DisableIRQ(btn1_intr_cfg.intrSrc);
    NVIC_DisableIRQ(btn2_intr_cfg.intrSrc);

    if (BTN_NOT_PRESSED == active_btn_state)
    {
        /* If the button was previously not pressed, set the current
         * state and start the long press timer. */
        active_btn_state = BTN_PRESSED;
        xTimerStopFromISR(btn_long_press_timer, NULL);
        xTimerResetFromISR(btn_long_press_timer, NULL);
        xTimerStartFromISR(btn_long_press_timer, NULL);
    }
    else
    {
        /* If the button was previously in pressed state, set the current
         * state and stop the timers. */
        xTimerStopFromISR(btn_debounce_timer, NULL);
        xTimerResetFromISR(btn_debounce_timer, NULL);
        xTimerStopFromISR(btn_long_press_timer, NULL);
        xTimerResetFromISR(btn_long_press_timer, NULL);

        /* If the long press timer did not elapse, handle the short press event. */
        if (BTN_PRESSED == active_btn_state)
        {
            btn_handle_short_press(active_btn_num);
        }

        active_btn_state = BTN_NOT_PRESSED;
        active_btn_num = USER_BTN_INVALID;
    }

    /* Start the debouncing timer in both cases when the button is pressed as
     * well as released. */
    xTimerStopFromISR(btn_debounce_timer, NULL);
    xTimerResetFromISR(btn_debounce_timer, NULL);
    xTimerStartFromISR(btn_debounce_timer, NULL);
}

/******************************************************************************
* Function Name: btn_debounce_timer_callback
*******************************************************************************
* Summary:
*   Timer callback for User Button debouncing.
*
* Parameters:
*   xTimer : Timer handle
*
* Return:
*   None
*
*******************************************************************************/
static void btn_debounce_timer_callback(TimerHandle_t xTimer)
{
    (void) xTimer;

    /* Enable GPIO pin interrupt for both buttons in case of button release,
     * else, enable the interrupt only for the active user button.
     */
    if ((BTN_NOT_PRESSED == active_btn_state) && (USER_BTN_INVALID == active_btn_num))
    {
        /* Clear the pending interrupts */
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

        /* Enable interrupts for both the user buttons */
        NVIC_EnableIRQ(btn1_intr_cfg.intrSrc);
        NVIC_EnableIRQ(btn2_intr_cfg.intrSrc);
    }
    else if (USER_BTN1 == active_btn_num)
    {
        /* Clear any pending user button 1 interrupt before enabling it */
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
        NVIC_EnableIRQ(btn1_intr_cfg.intrSrc);
    }
    else if (USER_BTN2 == active_btn_num)
    {
        /* Clear any pending user button 2 interrupt before enabling it */
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);
        NVIC_EnableIRQ(btn2_intr_cfg.intrSrc);
    }
}

/******************************************************************************
* Function Name: btn_long_press_timer_callback
*******************************************************************************
* Summary:
*   Timer callback for User Button Long Press detection.
*
* Parameters:
*   xTimer : Timer handle
*
* Return:
*   None
*
*******************************************************************************/
static void btn_long_press_timer_callback(TimerHandle_t xTimer)
{
    (void) xTimer;

    if (USER_BTN_INVALID == active_btn_num)
    {
        return;
    }

    if (CYBSP_BTN_PRESSED == Cy_GPIO_Read(active_btn_num == USER_BTN1 ?
                                          CYBSP_USER_BTN1_PORT : CYBSP_USER_BTN2_PORT,
                                          active_btn_num == USER_BTN1 ?
                                          CYBSP_USER_BTN1_PIN : CYBSP_USER_BTN2_PIN))
    {

        /* When the long press timer elapses, set the state appropriately and
         * handle the long press event. */
        if (BTN_PRESSED == active_btn_state)
        {
            active_btn_state = BTN_LONG_PRESS;
            btn_handle_long_press(active_btn_num);
        }
    }
    else
    {
        active_btn_state = BTN_NOT_PRESSED;
        active_btn_num = USER_BTN_INVALID;

        /* Clear the pending interrupts */
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

        /* Enable interrupts for both the user buttons */
        NVIC_EnableIRQ(btn1_intr_cfg.intrSrc);
        NVIC_EnableIRQ(btn2_intr_cfg.intrSrc);
    }
}

/******************************************************************************
* Function Name: btn_handle_short_press
*******************************************************************************
* Summary:
*   Function that performs action for user button short press
*
* Parameters:
*   btn_num : User button number for the button being pressed
*
* Return:
*   None
*
*******************************************************************************/
static inline void btn_handle_short_press(btn_num_t btn_num)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    btn_mgmt_cmd_t btn_mgmt_cmd = BTN_CMD_INVALID;

    if (USER_BTN_INVALID == active_btn_num)
    {
        return;
    }

    if (USER_BTN1 == btn_num)
    {
        btn_mgmt_cmd = BTN1_HANDLE_SHORT_PRESS_EVENT;
    }
    else if (USER_BTN2 == btn_num)
    {
        btn_mgmt_cmd = BTN2_HANDLE_SHORT_PRESS_EVENT;
    }
    xQueueSendFromISR(btn_mgmt_task_q, &btn_mgmt_cmd, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/******************************************************************************
* Function Name: btn_handle_long_press
*******************************************************************************
* Summary:
*   Function that performs action for user button long press
*
* Parameters:
*   btn_num : User button number for the button being pressed
*
* Return:
*   None
*
*******************************************************************************/
static inline void btn_handle_long_press(btn_num_t btn_num)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    btn_mgmt_cmd_t btn_mgmt_cmd = BTN_CMD_INVALID;

    if (USER_BTN_INVALID == active_btn_num)
    {
        return;
    }

    if (USER_BTN1 == btn_num)
    {
        btn_mgmt_cmd = BTN1_HANDLE_LONG_PRESS_EVENT;
    }
    else if (USER_BTN2 == btn_num)
    {
        btn_mgmt_cmd = BTN2_HANDLE_LONG_PRESS_EVENT;
    }
    xQueueSendFromISR(btn_mgmt_task_q, &btn_mgmt_cmd, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/******************************************************************************
* Function Name: btn_mgmt_task
*******************************************************************************
* Summary:
*   Task that logs user button events; broadcast audio controls are not implemented.
*
* Parameters:
*   pvParameters : Task parameter defined during task creation (unused)
*
* Return:
*   None
*
*******************************************************************************/
static void btn_mgmt_task(void* pvParameters)
{
    btn_mgmt_cmd_t btn_mgmt_cmd;

    /* Remove warning for unused parameter */
    (void)pvParameters;

    /* Create an RTOS queue to communicate data / commands between various tasks
     * and callbacks.
     */
    btn_mgmt_task_q = xQueueCreate(BTN_MGMT_TASK_QUEUE_LENGTH,
                                   sizeof(btn_mgmt_cmd_t));

    printf("Button Management Task started...\n");

    for (;;)
    {
        /* Receive the command and data from the queue and process it. */
        if (pdTRUE == xQueueReceive(btn_mgmt_task_q, &btn_mgmt_cmd, portMAX_DELAY))
        {
                switch(btn_mgmt_cmd)
                {
                    case BTN1_HANDLE_SHORT_PRESS_EVENT:
                    WICED_BT_TRACE("[btn_mgmt] BTN1 short press (volume down - N/A for broadcast)\n");
                    break;
                
                    case BTN1_HANDLE_LONG_PRESS_EVENT:
                    WICED_BT_TRACE("[btn_mgmt] BTN1 long press (play/pause - N/A for broadcast)\n");
                    break;
                
                    case BTN2_HANDLE_SHORT_PRESS_EVENT:
                    WICED_BT_TRACE("[btn_mgmt] BTN2 short press (volume up - N/A for broadcast)\n");
                    break;
                
                    case BTN2_HANDLE_LONG_PRESS_EVENT:
                    WICED_BT_TRACE("[btn_mgmt] BTN2 long press (mute - N/A for broadcast)\n");
                    break;

                    default:
                    {
                        printf(">> Error: Unsupported button management command %d\n",
                            btn_mgmt_cmd);
                    }
                }
        }
    }
}

/* [] END OF FILE */
