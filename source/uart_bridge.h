#ifndef UART_BRIDGE_H_
#define UART_BRIDGE_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "sensor.h"

void UART_Bridge_Init(uint32_t baud_rate);
void UART_Bridge_SetSensorDataHandle(SensorData_t *sharedData, SemaphoreHandle_t dataMutex);
void UART_Bridge_StartTasks(UBaseType_t recvPriority, UBaseType_t pollPriority);
BaseType_t UART_Bridge_Send(const char *msg);
BaseType_t UART_Bridge_SendSensorTelemetry(const SensorData_t *data);

#endif /* UART_BRIDGE_H_ */
