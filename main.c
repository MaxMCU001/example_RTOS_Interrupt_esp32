#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "freertos/semphr.h"


#define GPIO_OUTPUT_IO_0    1
#define GPIO_OUTPUT_IO_1    23
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

#define	GPIO_INPUT_IO_0		4
#define	GPIO_INPUT_IO_1		5
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))

#define ESP_INTR_FLAG_DEFAULT 0


SemaphoreHandle_t interruptSemaphore1;			// Create semaphore handle
SemaphoreHandle_t interruptSemaphore2;

void IRAM_ATTR keyISR1(void* arg)		// ISR definition
{
	xSemaphoreGiveFromISR(interruptSemaphore1,NULL);
}

void IRAM_ATTR keyISR2(void* arg)
{
	xSemaphoreGiveFromISR(interruptSemaphore2,NULL);
}

void blink1(void *pvParameters)
{
	while(1)
	{
		if(xSemaphoreTake(interruptSemaphore1,portMAX_DELAY))
		{
			for(int i = 0; i < 10; i++)
			{
				gpio_set_level(GPIO_OUTPUT_IO_0, 1);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				gpio_set_level(GPIO_OUTPUT_IO_0, 0);
				vTaskDelay(50 / portTICK_PERIOD_MS);
			}
		}
	}
}
void blink2(void *pvParameters)
{
	while(1)
	{
		if(xSemaphoreTake(interruptSemaphore2, portMAX_DELAY))
		{
			for(int i = 0; i < 10; i++)
			{
				gpio_set_level(GPIO_OUTPUT_IO_1, 1);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				gpio_set_level(GPIO_OUTPUT_IO_1, 0);
				vTaskDelay(50 / portTICK_PERIOD_MS);
			}
		}
	}
}

void app_main(void)
{
	esp_task_wdt_deinit();

	gpio_config_t io_conf = {};

	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	// disable pull-up mode
	io_conf.pull_up_en = 0;
	// configure GPIO with the given settings
	gpio_config(&io_conf);

	// interrupt of rising edge
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	// bit mask of the pins, use GPIO4/5 here
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	// set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE);
	gpio_set_intr_type(GPIO_INPUT_IO_1, GPIO_INTR_NEGEDGE);
	// install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	interruptSemaphore1 = xSemaphoreCreateBinary();	// Create semaphore
	interruptSemaphore2 = xSemaphoreCreateBinary();

	xTaskCreate(
			blink1,				// Function name of the task
			"Blink 1",			// Name of the task (e.g. for debugging)
			2048,				// Stack size (bytes)
			NULL,				// Parameter to pass
			1,					// Task priority
			NULL				// Task handle
			);
	xTaskCreate(
			blink2,				// Function name of the task
			"Blink 2",			// Name of the task (e.g. for debugging)
			2048,				// Stack size (bytes)
			NULL,				// Parameter to pass
			1,					// Task priority
			NULL				// Task handle
			);

	if(interruptSemaphore1 != NULL)
	{
		// hook isr handler for specific gpio pin
		gpio_isr_handler_add(GPIO_INPUT_IO_0,keyISR1,(void*)GPIO_INPUT_IO_0);
	}

	if(interruptSemaphore2 != NULL)
	{
		// hook isr handler for specific gpio pin
		gpio_isr_handler_add(GPIO_INPUT_IO_1,keyISR2,(void*)GPIO_INPUT_IO_1);
	}
}
