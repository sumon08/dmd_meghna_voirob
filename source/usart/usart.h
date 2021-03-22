



#ifndef USART_H
#define USART_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

typedef void * UsartHandle;

typedef enum
{
	COM1,
	COM2,
	COM3,
	COM4,
	COM5,
	COM6
}ComId;



typedef struct
{
	ComId port_id;
	uint32_t baud;
	QueueHandle_t tx_buffer;
	QueueHandle_t rx_buffer;
	uint32_t is_initialised;
}UsartType;

UsartHandle InitUsart(ComId id, uint32_t baud, uint32_t tx_buffer_limit, uint32_t rx_buffer_limit);
void UsartSendByte(UsartHandle handle, uint8_t data);
void UsartSendString(UsartHandle handle, const char * string);
uint32_t UsartReceiveByte(UsartHandle handle, uint8_t * data);
void ResetUsartBuffer(UsartHandle handle);
uint32_t IsInitialised(UsartHandle handle);



#endif //USAR_H
