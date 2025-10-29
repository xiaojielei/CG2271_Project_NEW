/*
 * sensor_driver.c
 *
 *  Created on: Oct 29, 2025
 *      Author: 86178
 */


#include "sensor_driver.h"

void ADC_Config(void);
void init_WaterLevel_ISR(void);


void Sensors_Init(void) {
   // Step 0: Enable clock for PORTE and PORTC
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // Step 1: Configure water level sensor pin (PTE23)
    PORTE->PCR[WATER_LEVEL_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[WATER_LEVEL_PIN] |= PORT_PCR_MUX(0);  // ALT0 for analog

    // Configure port for analog functionality (disable digital)
    PORTE->PCR[WATER_LEVEL_PIN] &= ~PORT_PCR_DSE_MASK;

    // Step 2: Configure photoresistor pin (PTE22 - ADC0_SE3)
    PORTE->PCR[22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[22] |= PORT_PCR_MUX(0);  // ALT0 for ADC

    // Step 3: Configure ADC for photoresistor
    ADC_Config();

    // Step 4: Configure water level interrupt
    init_WaterLevel_ISR();

    // Print initialization message
    #ifdef SENSOR_DEBUG
    PRINTF("Sensors initialized successfully.\r\n");
    PRINTF("Water Level: Dry=%d, Wet=%d\r\n", water_level_dry, water_level_wet);
    #endif
}

void ADC_Config(void) {
    // Step 1: Enable ADC clock gating (SIM_SCGC6)
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // Step 2: Enable clock for PORTE (for photoresistor pin PTE22)
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Step 3: Configure ADC pin (PTE22 for photoresistor - ADC0_SE3)
    PORTE->PCR[22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[22] |= PORT_PCR_MUX(0);

    // Step 4: Configure ADC0_CFG1 for 12-bit resolution
    ADC0->CFG1 &= ~ADC_CFG1_MODE_MASK;
    ADC0->CFG1 |= ADC_CFG1_MODE(0b01);

    // Step 5: Configure ADC0_SC2 for software trigger and alternate reference
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;
    ADC0->SC2 &= ~ADC_SC2_REFSEL_MASK;
    ADC0->SC2 |= ADC_SC2_REFSEL(0b01);

    // Step 6: Configure ADC0_SC3 - disable averaging, single conversion
    ADC0->SC3 &= ~ADC_SC3_AVGE_MASK;
    ADC0->SC3 |= ADC_SC3_AVGE(0);

    ADC0->SC3 &= ~ADC_SC3_ADCO_MASK;
    ADC0->SC3 |= ADC_SC3_ADCO(0);

    // Step 7: Configure ADC0_SC1A for polling mode (no interrupts)
    ADC0->SC1[0] &= ~ADC_SC1_AIEN_MASK;
    ADC0->SC1[0] &= ~ADC_SC1_DIFF_MASK;
    ADC0->SC1[0] |= ADC_SC1_DIFF(0b0);
}

void init_WaterLevel_ISR(void) {

	NVIC_ClearPendingIRQ(PORTA_IRQn);

    // Step 1: Enable clock for PORTE
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Step 2: Configure water level pin (PTA4) for GPIO with interrupt
    PORTE->PCR[WATER_LEVEL_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[WATER_LEVEL_PIN] |= PORT_PCR_MUX(1);  // ALT1 for GPIO

    // Step 3: Enable and set pull-up resistor and set interrupt to falling edge triggered
    PORTE->PCR[WATER_LEVEL_PIN] |= (PORT_PCR_PE_MASK |   // Pull enable
                                   PORT_PCR_PS_MASK |    // Pull-up
                                   PORT_PCR_IRQC(0x0A)); // Falling edge interrupt (0b1010)

    // Step 4: Set pin as input in GPIO
    GPIOE->PDDR &= ~(1 << WATER_LEVEL_PIN);

    // Step 5: set interrupt priority
    NVIC_SetPriority(PORTA_IRQn, 2);  // Higher priority for water level

    //Step6: enable interrupt
    NVIC_EnableIRQ(PORTA_IRQn);
}

//configure ADC for water level and read the analog value
uint32_t Read_WaterLevel(void) {
    uint32_t adcValue;
    uint32_t waterLevelPercent;

    // Configure ADC0_SC1A to start conversion on water level channel
    // Assuming water level is on ADC0_SE2 (channel 2)
    ADC0->SC1[0] &= ~ADC_SC1_ADCH_MASK;
    ADC0->SC1[0] |= ADC_SC1_ADCH(2);

    // Poll for conversion complete (COCO flag in SC1A)
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)) {
        // Wait for conversion to complete
    }

    // Read ADC value
    adcValue = ADC0->R[0];

    // Convert to percentage (calibrated)
    if (adcValue <= water_level_dry) {
        waterLevelPercent = 0;
    } else if (adcValue >= water_level_wet) {
        waterLevelPercent = 100;
    } else {
        waterLevelPercent = (adcValue - water_level_dry) * 100 / (water_level_wet - water_level_dry);
    }

    return waterLevelPercent;
}

uint32_t Read_Photoresistor(void) {
    uint32_t adcValue;

    // Configure ADC0_SC1A to start conversion on photoresistor channel
    // Photoresistor is on ADC0_SE3 (channel 3)
    ADC0->SC1[0] &= ~ADC_SC1_ADCH_MASK;
    ADC0->SC1[0] |= ADC_SC1_ADCH(3);

    // Poll for conversion complete (COCO flag in SC1A)
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)) {
        // Wait for conversion to complete
    }

    // Read ADC value
    adcValue = ADC0->R[0];

    return adcValue;
}

// Function to get the last measured water level
uint32_t Get_Last_WaterLevel(void) {
    return last_water_level;
}
