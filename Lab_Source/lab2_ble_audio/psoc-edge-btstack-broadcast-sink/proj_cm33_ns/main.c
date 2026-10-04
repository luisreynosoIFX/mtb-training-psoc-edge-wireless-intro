/******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for Non-Secure
*                    Application in the CM33 CPU.
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


/******************************************************************************
* Macros
*******************************************************************************/
/* The timeout value in microsecond used to wait for core to be booted */
#define CM55_BOOT_WAIT_TIME_US          (10U)

/* The time in ms to wait post CM55 enablement for SoCMEM configuration */
#define SOCMEM_CONFIG_WAIT_TIME_MS      (150U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR              (CYMEM_CM33_0_m55_nvm_START + \
                                         CYBSP_MCUBOOT_HEADER_SIZE)


/******************************************************************************
* Function Name: main
*******************************************************************************
* Summary:
*   This is the main function of the CM33 non-secure application.
*
*   It initializes the device, enables the CM55 CPU, configures the idle power
*   modes, and enters deep sleep.
*
* Parameters:
*   None
*
* Return:
*   int
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    (void) result;

    /* Initialize the device and board peripherals. Halt if failed. */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Enable global interrupts */
    __enable_irq();

    /* Enable the CM55 CPU. CM55_APP_BOOT_ADDR must be updated if the
     * CM55 memory layout is changed.
     */
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_US);

    /* Wait for CM55 to boot up before configuring SoCMEM */
    Cy_SysLib_Delay(SOCMEM_CONFIG_WAIT_TIME_MS);

    /* System Domain Idle Power Mode Configuration */
    Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

    /* SoCMEM Idle Power Mode Configuration */
    Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

    for (;;)
    {
        Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
}

/* [] END OF FILE */
