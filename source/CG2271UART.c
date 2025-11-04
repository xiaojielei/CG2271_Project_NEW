/*
 * @file    uart_bridge.c
 * @brief   UART2 FreeRTOS bridge – exchanges data with ESP32
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "sensor.h"
#include "uart_bridge.h"

#define UART_TX_PTE22   22
#define UART_RX_PTE23   23
#define UART2_INT_PRIO  128
#define MAX_MSG_LEN     128
#define RX_QUEUE_LEN    5

typedef struct {
    char message[MAX_MSG_LEN];
} UartMessage_t;

static QueueHandle_t rxQueue;
static SemaphoreHandle_t txMutex;

static SensorData_t *gSensorData;
static SemaphoreHandle_t gSensorDataMutex;

static void initUART2(uint32_t baud_rate);
static void handle_incoming_payload(const char *payload);
static BaseType_t uart_send_locked(const char *msg);
static void uart_request_task(void *pv);
static void uart_receive_task(void *pv);

void UART_Bridge_Init(uint32_t baud_rate)
{
    rxQueue = xQueueCreate(RX_QUEUE_LEN, sizeof(UartMessage_t));
    configASSERT(rxQueue != NULL);

    txMutex = xSemaphoreCreateMutex();
    configASSERT(txMutex != NULL);

    gSensorData = NULL;
    gSensorDataMutex = NULL;

    initUART2(baud_rate);
}

void UART_Bridge_SetSensorDataHandle(SensorData_t *sharedData, SemaphoreHandle_t dataMutex)
{
    gSensorData = sharedData;
    gSensorDataMutex = dataMutex;
}

void UART_Bridge_StartTasks(UBaseType_t recvPriority, UBaseType_t pollPriority)
{
    configASSERT(rxQueue != NULL);
    xTaskCreate(uart_receive_task, "UART-RX", configMINIMAL_STACK_SIZE + 256, NULL, recvPriority, NULL);
    xTaskCreate(uart_request_task, "UART-TX", configMINIMAL_STACK_SIZE + 128, NULL, pollPriority, NULL);
}

BaseType_t UART_Bridge_Send(const char *msg)
{
    return uart_send_locked(msg);
}

BaseType_t UART_Bridge_SendSensorTelemetry(const SensorData_t *data)
{
    if (data == NULL) {
        return pdFAIL;
    }

    char buffer[MAX_MSG_LEN];
    int written = snprintf(buffer, sizeof(buffer),
                           "{\"photo\":%lu,\"water\":%lu}\n",
                           (unsigned long)data->light_intensity,
                           (unsigned long)data->water_level);
    if (written <= 0 || written >= (int)sizeof(buffer)) {
        return pdFAIL;
    }
    return uart_send_locked(buffer);
}

static void initUART2(uint32_t baud_rate)
{
    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

    PORTE->PCR[UART_TX_PTE22] = PORT_PCR_MUX(4);
    PORTE->PCR[UART_RX_PTE23] = PORT_PCR_MUX(4);

    uint32_t bus_clk = CLOCK_GetBusClkFreq();
    uint32_t sbr = (bus_clk + (baud_rate * 8u)) / (baud_rate * 16u);
    UART2->BDH = (UART2->BDH & ~UART_BDH_SBR_MASK) | ((sbr >> 8) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFFu);

    UART2->C1 = 0x00; // 8N1
    UART2->C2 = UART_C2_RIE_MASK | UART_C2_RE_MASK | UART_C2_TE_MASK; // Enable RX interrupt + RX/TX

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);
}

static BaseType_t uart_send_locked(const char *msg)
{
    if (msg == NULL || txMutex == NULL) {
        return pdFAIL;
    }

    if (xSemaphoreTake(txMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return pdFAIL;
    }

    const char *p = msg;
    while (*p != '\0') {
        while (!(UART2->S1 & UART_S1_TDRE_MASK)) {
        }
        UART2->D = *(p++);
    }
    while (!(UART2->S1 & UART_S1_TC_MASK)) {
    }

    xSemaphoreGive(txMutex);
    return pdPASS;
}

void UART2_FLEXIO_IRQHandler(void)
{
    static size_t recv_index = 0;
    static char recv_buffer[MAX_MSG_LEN];
    BaseType_t hpw = pdFALSE;

    if (UART2->S1 & UART_S1_RDRF_MASK) {
        char rxByte = UART2->D;
        if (recv_index < (MAX_MSG_LEN - 1u)) {
            recv_buffer[recv_index++] = rxByte;
        }

        if (rxByte == '\n') {
            recv_buffer[recv_index] = '\0';
            UartMessage_t message;
            strncpy(message.message, recv_buffer, MAX_MSG_LEN);
            if (rxQueue != NULL) {
                xQueueSendFromISR(rxQueue, &message, &hpw);
            }
            recv_index = 0u;
        }
    }

    portYIELD_FROM_ISR(hpw);
}

static void uart_receive_task(void *pv)
{
    (void)pv;
    UartMessage_t message;
    while (1) {
        if (xQueueReceive(rxQueue, &message, portMAX_DELAY) == pdTRUE) {
            PRINTF("From ESP32: %s\r\n", message.message);
            handle_incoming_payload(message.message);
        }
    }
}

static void uart_request_task(void *pv)
{
    (void)pv;
    while (1) {
        uart_send_locked("GET_DHT\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void handle_incoming_payload(const char *payload)
{
    if (payload == NULL) {
        return;
    }

    const char *tempPos = strstr(payload, "\"temperature\"");
    if (tempPos == NULL) {
        tempPos = strstr(payload, "\"temp\"");
    }
    const char *humPos = strstr(payload, "\"humidity\"");
    if (humPos == NULL) {
        humPos = strstr(payload, "\"hum\"");
    }
    if (!tempPos || !humPos) {
        return;
    }

    tempPos = strchr(tempPos, ':');
    humPos = strchr(humPos, ':');
    if (!tempPos || !humPos) {
        return;
    }

    float temperature = strtof(tempPos + 1, NULL);
    float humidity = strtof(humPos + 1, NULL);

    Sensor_UpdateRemoteReadings(temperature, humidity);
    PRINTF("ESP32 DHT -> temp: %.2f C, humidity: %.2f %%\r\n", temperature, humidity);
}
