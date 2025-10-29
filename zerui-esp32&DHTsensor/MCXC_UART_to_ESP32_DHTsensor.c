/*
 * @file    CG2271RTOS_UART.c
 * @brief   UART2 FreeRTOS example – Communicates with ESP32 DHT bridge
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define BAUD_RATE 9600
#define UART_TX_PTE22 22
#define UART_RX_PTE23 23
#define UART2_INT_PRIO 128
#define MAX_MSG_LEN 128
#define QLEN 5

typedef struct
{
    char message[MAX_MSG_LEN];
} TMessage;

static char send_buffer[MAX_MSG_LEN];
static QueueHandle_t queue;

// ───────────────────────────────────────────────
// UART2 Initialization
// ───────────────────────────────────────────────
void initUART2(uint32_t baud_rate)
{
    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

    PORTE->PCR[UART_TX_PTE22] = PORT_PCR_MUX(4);
    PORTE->PCR[UART_RX_PTE23] = PORT_PCR_MUX(4);

    uint32_t bus_clk = CLOCK_GetBusClkFreq();
    uint32_t sbr = (bus_clk + (baud_rate * 8)) / (baud_rate * 16);
    UART2->BDH = (UART2->BDH & ~UART_BDH_SBR_MASK) | ((sbr >> 8) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFF);

    UART2->C1 = 0x00;                               // 8N1
    UART2->C2 = UART_C2_RIE_MASK | UART_C2_RE_MASK; // Enable RX interrupt + receiver

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);
}

// ───────────────────────────────────────────────
// UART ISR
// ───────────────────────────────────────────────
void UART2_FLEXIO_IRQHandler(void)
{
    static int recv_ptr = 0, send_ptr = 0;
    static char recv_buffer[MAX_MSG_LEN];
    char rx_data;
    BaseType_t hpw = pdFALSE;

    // TX ready
    if (UART2->S1 & UART_S1_TDRE_MASK)
    {
        if (send_buffer[send_ptr] == '\0')
        {
            send_ptr = 0;
            UART2->C2 &= ~(UART_C2_TIE_MASK | UART_C2_TE_MASK);
        }
        else
        {
            UART2->D = send_buffer[send_ptr++];
        }
    }

    // RX ready
    if (UART2->S1 & UART_S1_RDRF_MASK)
    {
        rx_data = UART2->D;
        if (recv_ptr < MAX_MSG_LEN - 1)
            recv_buffer[recv_ptr++] = rx_data;

        if (rx_data == '\n')
        {
            recv_buffer[recv_ptr] = '\0';
            TMessage msg;
            strncpy(msg.message, recv_buffer, MAX_MSG_LEN);
            xQueueSendFromISR(queue, &msg, &hpw);
            recv_ptr = 0;
            portYIELD_FROM_ISR(hpw);
        }
    }
}

// ───────────────────────────────────────────────
// Helper to transmit string via interrupt
// ───────────────────────────────────────────────
void sendMessage(const char *msg)
{
    strncpy(send_buffer, msg, MAX_MSG_LEN);
    UART2->C2 |= UART_C2_TIE_MASK | UART_C2_TE_MASK;
}

// ───────────────────────────────────────────────
// Parse JSON {"temp":27.5,"humidity":62.0}
// ───────────────────────────────────────────────
void parseJSON(const char *json, float *temp, float *hum)
{
    const char *tp = strstr(json, "\"temp\":");
    const char *hp = strstr(json, "\"humidity\":");
    *temp = *hum = -99.0f;
    if (tp)
        *temp = atof(tp + 7);
    if (hp)
        *hum = atof(hp + 11);
}

// ───────────────────────────────────────────────
// Task: Receive responses from ESP32
// ───────────────────────────────────────────────
static void recvTask(void *p)
{
    TMessage msg;
    while (1)
    {
        if (xQueueReceive(queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            float t, h;
            parseJSON(msg.message, &t, &h);
            PRINTF("From ESP32: %s\r\n", msg.message);
            if (t > -90 && h > -90)
                printf("Temp: %.1f °C, Humidity: %.1f %%\r\n", t, h);
        }
    }
}

// ───────────────────────────────────────────────
// Task: Request DHT data from ESP32
// ───────────────────────────────────────────────
static void sensorTask(void *p)
{
    while (1)
    {
        sendMessage("GET_DHT\n");
        vTaskDelay(pdMS_TO_TICKS(2000)); // every 2 seconds
    }
}

// ───────────────────────────────────────────────
// Main
// ───────────────────────────────────────────────
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    PRINTF("FRDM UART2 FreeRTOS ESP32 DHT Demo\r\n");

    initUART2(BAUD_RATE);
    queue = xQueueCreate(QLEN, sizeof(TMessage));

    xTaskCreate(recvTask, "recvTask", configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);
    xTaskCreate(sensorTask, "sensorTask", configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1)
        ;
}
