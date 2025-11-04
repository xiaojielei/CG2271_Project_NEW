#include "board.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "actuator_driver.h"
#define LED_PIN_PTC   1u
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    Actuators_Init();              // 1) set up LED + buzzer
    //Actuators_Init();
    SDK_DelayAtLeastUs(10000U, SystemCoreClock);
    Set_LED_Intensity(255);        // test brightness
    Play_Music(MUSIC_HAPPY);       // test tune

    SDK_DelayAtLeastUs(500000U, SystemCoreClock);
    Play_Music(MUSIC_SAD);
    Set_LED_Intensity(20);
    while (1) {
        __NOP();
    }
}
