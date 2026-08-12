//#############################################################################
//
// F280049C_Signal - bare-device SysConfig project template
//
//#############################################################################

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"
#include "sigf32.h"  // Algorithm API; instantiate states in the ADC/DMA callback.

void main(void)
{
    // Initialize the device clock and basic system resources.
    Device_init();

    // Unlock GPIO configuration and enable the device defaults.
    Device_initGPIO();

    // Initialize the interrupt controller and vector table.
    Interrupt_initModule();
    Interrupt_initVectorTable();

    // Apply the pin, clock, and peripheral configuration from main.syscfg.
    Board_init();
    C2000Ware_libraries_init();

    EINT;
    ERTM;

    // Add ADC ISR or DMA block processing here; the library has no GPIO dependency.

    for (;;)
    {
    }
}

