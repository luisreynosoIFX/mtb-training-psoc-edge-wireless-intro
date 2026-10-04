/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for DEEPCRAFT Deploy Motion Example.
*
* Related Document : See README.md
*
*******************************************************************************
* (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
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

#ifdef ML_DEEPCRAFT_CM33
#include "stdlib.h"
#include "retarget_io_init.h"

#include "imu.h"
#endif /* ML_DEEPCRAFT_CM33 */

#include "cy_time.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"
#include "cy_vcm.h"
#include "retarget_io_init.h"
#include "mqtt_task.h"
#include "safe_log.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* The timeout value in microsecond used to wait for core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC    (10U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)

/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles
 * to come into effect. This wait time value will depend on the actual CLK_LF
 * frequency set by the BSP.
 */
#define LPTIMER_0_WAIT_TIME_USEC            (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority.
 */
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;

/* RTC HAL object */
static mtb_hal_rtc_t rtc_obj;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
#ifdef ML_DEEPCRAFT_CM33
static cy_rslt_t system_init(void);
static void cm33_ml_deepcraft_task(void);
#endif /* ML_DEEPCRAFT_CM33 */
static void setup_clib_support(void);
static void setup_tickless_idle_timer(void);
static void vcm_callback(cy_vcm_event_t event);

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  This is the main function. It initializes the system, the Virtual
*  Connectivity Manager (VCM) and boots the CM55 CPU.
*  If the model inferencing is set to CM33 + NNLite, it starts the Deploy
*  Motion application, else, it starts the MQTT client task. CM33 is the
*  primary connectivity core: it runs the Wi-Fi and MQTT stack, and shares
*  the MQTT connection with CM55 through VCM.
*
* Parameters:
*  None
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_vcm_config_t primary_core_config;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Setup CLIB support library. */
    setup_clib_support();

    /* Setup the LPTimer instance for CM33 CPU. */
    setup_tickless_idle_timer();

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize the debug UART used by retarget-io and safe_print(). */
    init_retarget_io();
    /* TODO 1: Initialize VCM before booting CM55. CM33 boots first, so it
     * creates the IPC resources that CM55 uses.
     *  - Fill in primary_core_config: ipc_obj = &cybsp_cm33_ipc_instance,
     *    hal_resource_opt = CY_VCM_CREATE_HAL_RESOURCE,
     *    channel_num = MTB_IPC_CHAN_1, event_cb = vcm_callback.
     *  - Call cy_vcm_init() and call handle_app_error() if it fails.
     */

    /* Enable CM55. */
    /* CY_CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

#ifdef ML_DEEPCRAFT_CM33
    /* If ML_DEEPCRAFT_CPU is set as CM33, start the deploy application */
    cm33_ml_deepcraft_task();
#else

    /* If ML_DEEPCRAFT_CPU is set as CM55, CM33 handles Wi-Fi and MQTT */
    if (pdPASS == xTaskCreate(mqtt_client_task, "MQTT Client task",
                              MQTT_CLIENT_TASK_STACK_SIZE, NULL,
                              MQTT_CLIENT_TASK_PRIORITY, NULL))
    {
        /* Start the RTOS Scheduler */
        vTaskStartScheduler();
    }

    /* Should never get here. */
    handle_app_error();
#endif /* ML_DEEPCRAFT_CM33 */
}

/*******************************************************************************
* Function Name: setup_clib_support
********************************************************************************
* Summary:
*    1. This function configures and initializes the Real-Time Clock (RTC).
*    2. It then initializes the RTC HAL object to enable CLIB support library
*       to work with the provided Real-Time Clock (RTC) module.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_clib_support(void)
{
    /* RTC Initialization */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);

    /* Initialize the ModusToolbox CLIB support library */
    mtb_clib_support_init(&rtc_obj);
}

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for LPTimer instance.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
*    1. This function first configures and initializes an interrupt for LPTimer.
*    2. Then it initializes the LPTimer HAL object to be used in the RTOS
*       tickless idle mode implementation to allow the device enter deep sleep
*       when idle task runs. LPTIMER_0 instance is configured for CM33 CPU.
*    3. It then passes the LPTimer object to abstraction RTOS library that
*       implements tickless idle mode
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status =
                                    Cy_SysInt_Init(&lptimer_intr_cfg,
                                                    lptimer_interrupt_handler);

    /* LPTimer interrupt initialization failed. Stop program execution. */
    if (CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status =
                                    Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW,
                                                &CYBSP_CM33_LPTIMER_0_config);

    /* MCWDT initialization failed. Stop program execution. */
    if (CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }

    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
                    CY_MCWDT_CTR_Msk,
                    LPTIMER_0_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj,
                                            &CYBSP_CM33_LPTIMER_0_hal_config);

    /* LPTimer setup failed. Stop program execution. */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements
     * tickless idle mode
     */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}

/*******************************************************************************
* Function Name: vcm_callback
********************************************************************************
* Summary:
*  Callback invoked by VCM library for events like Virtualization init or deinit
*
* Parameters:
*  cy_vcm_event_t event : VCM event information
*
* Return:
*  void
*******************************************************************************/
static void vcm_callback(cy_vcm_event_t event)
{
    switch(event)
    {
        case CY_VCM_EVENT_INIT_COMPLETE:
        {
            safe_print("\nVCM Callback: received INIT_COMPLETE from CM55\r\n");
            break;
        }
        case CY_VCM_EVENT_DEINIT:
        {
            safe_print("\nVCM Callback: received DEINIT from CM55\r\n");
            break;
        }
        default:
        {
            safe_print("\nVCM Callback: Unknown event\r\n");
            break;
        }
    }
}

#ifdef ML_DEEPCRAFT_CM33
/*******************************************************************************
* Function Name: system_init
********************************************************************************
* Summary:
*  Initializes the neural network based on the DEEPCRAFT model and the
*  DEEPCRAFT pre-processor and initializes the IMU sensor.
*
* Parameters:
*  None
*
* Returns:
*  The status of the initialization.
*
*******************************************************************************/
static cy_rslt_t system_init(void)
{
    cy_rslt_t result;

    /* Initialize DEEPCRAFT pre-processing library */
    IMAI_init();

    /* Initialize the IMU and related interrupt handling code */
    result = imu_init();

    return result;
}

/*******************************************************************************
* Function Name: cm33_ml_deepcraft_task
********************************************************************************
* Summary:
*  Contains the main loop for the application. It sets up the UART for
*  logs and initialises the system (DEEPCRAFT pre-processor and IMU for input
*  data). It then invokes the IMU Data Processing function that sends the data
*  for pre-processing, inferencing, and prints in the results when enough data
*  data is received.
*
* Parameters:
*  None
*
* Returns:
*  None
*
*******************************************************************************/
static void cm33_ml_deepcraft_task(void)
{
    cy_rslt_t result;

    /* Initialize inference engine and sensors */
    result = system_init();

    /* Initialization failed */
    if(CY_RSLT_SUCCESS != result)
    {
        /* Failed to initialize properly */
        safe_print("System initialization fail\r\n");
        while(1);
    }

    safe_print("DEEPCRAFT Studio Deploy Motion Example - CM33\r\n");

    for (;;)
    {
        /* Invoke the IMU Data Processing function that sends the data for
         * pre-processing, inferencing, and print the results when enough data
         * is received.
         */
        imu_data_process();
    }
}
#endif /* ML_DEEPCRAFT_CM33 */

/* [] END OF FILE */
