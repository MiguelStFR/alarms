#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_rom_gpio.h"
#include "rom/gpio.h"

//QUEUE MODEL
QueueHandle_t  bufferMovement, bufferEmergency;  //Objeto da queue

// Defina os números dos pinos para o botão e o LED
#define buttonEmergency 25
#define buttonMovement 26

// Defina os números dos pinos para o LED RGB
#define redPin 13
#define greenPin 14
#define bluePin 12

void taskAlarm(void* z) {  //Tarefa que simula PWM para observar no analisador logico
  unsigned int rcv, rcv2;
  rcv = rcv2 = 0;

  esp_rom_gpio_pad_select_gpio(redPin);
  esp_rom_gpio_pad_select_gpio(greenPin);
  esp_rom_gpio_pad_select_gpio(bluePin);
  
  //Botões que simulam alarme
  gpio_set_direction(redPin, GPIO_MODE_OUTPUT);
  gpio_set_direction(greenPin, GPIO_MODE_OUTPUT);
  gpio_set_direction(bluePin, GPIO_MODE_OUTPUT);

  //0 = parado = verde
  gpio_set_level(redPin, 0);
  gpio_set_level(greenPin, 1);
  gpio_set_level(bluePin, 0);

  if(bufferMovement == NULL){
    bufferMovement = xQueueCreate(10, sizeof(unsigned int));  //Cria a queue *buffer* com 100 slots de 4 Bytes
  }
  if(bufferEmergency == NULL){
    bufferEmergency = xQueueCreate(10, sizeof(unsigned int));  //Cria a queue *buffer* com 100 slots de 4 Bytes
  }

  while (1) {
    if (xQueueReceive(bufferEmergency, &rcv, pdMS_TO_TICKS(1000)) == true)  //Se recebeu o valor dentro de 1seg (timeout), mostrara na tela
    {
      ESP_LOGI("Queue", "taskAlarm leitura bufferEmergency -> Item recebido: %u", rcv);  //Mostra o valor recebido na tela
      if (rcv == 1) {
        //1 = emergência = vermelho
        gpio_set_level(redPin, 1);
        gpio_set_level(greenPin, 0);
        gpio_set_level(bluePin, 0);
      }else{
        if (xQueueReceive(bufferMovement, &rcv2, pdMS_TO_TICKS(1000)) == true){
          ESP_LOGI("Queue", "taskAlarm leitura bufferMovement -> Item recebido: %u", rcv2);  //Mostra o valor recebido na tela
          if(rcv2 == 1){
            //1 = Movimentando azul
            gpio_set_level(redPin, 0);
            gpio_set_level(greenPin, 0);
            gpio_set_level(bluePin, 1);
          } else {
            //0 = parado = verde
            gpio_set_level(redPin, 0);
            gpio_set_level(greenPin, 1);
            gpio_set_level(bluePin, 0);
          }
        }else{
          printf("taskAlarm: Nenhuma leitura.\n");
        }
      } 
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void taskMovement(void* z) {  //Tarefa que simula PWM para observar no analisador logico
  unsigned int snd;

  if(bufferMovement == NULL){
    bufferMovement = xQueueCreate(10, sizeof(unsigned int));  //Cria a queue *buffer* com 10 slots de 4 Bytes
  }

  //Botões que simulam tasks externas
  gpio_pad_select_gpio(buttonMovement);
  gpio_set_direction(buttonMovement, GPIO_MODE_INPUT);

  while (1) {
    snd = gpio_get_level(buttonMovement);
    if (snd == 1) {
      //Movimentando
      xQueueSendToBack(bufferMovement, &snd, pdMS_TO_TICKS(1000));  //Envia a variavel para queue
      ESP_LOGI("Queue", "ntaskMovement -> Item recebido: %u", snd);
    } else {
      //Parado
      xQueueSendToBack(bufferMovement, &snd, pdMS_TO_TICKS(1000));  //Envia a variavel para queue
      ESP_LOGI("Queue", "ntaskMovement -> Item recebido: %u", snd);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void taskEmergency(void* z) {  //Tarefa que simula PWM para observar no analisador logico

  gpio_pad_select_gpio(buttonEmergency);
  gpio_set_direction(buttonEmergency, GPIO_MODE_INPUT);
  
  unsigned int snd = 1;    
  //unsigned int aux = 0;
  if(bufferEmergency == NULL){
    bufferEmergency = xQueueCreate(10, sizeof(unsigned int));  //Cria a queue *buffer* com 10 slots de 4 Bytes
  }

  while (1) {

    if(gpio_get_level(buttonEmergency) == 1){
      snd = 1;
      xQueueSendToBack(bufferEmergency, &snd, pdMS_TO_TICKS(1000));  //Envia a variavel para queue
      ESP_LOGI("Queue", "ntaskEmergency -> Estado Atual: %u", snd);
    }else{
      snd = 0;
      xQueueSendToBack(bufferEmergency, &snd, pdMS_TO_TICKS(1000));  //Envia a variavel para queue
      ESP_LOGI("Queue", "ntaskEmergency -> Estado Atual: %u", snd);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void app_main(void)
{
  printf("app_main");   
  //QUEUE MODEL
  //Cria as tarefas
  xTaskCreate(&taskAlarm, "taskAlarm", 2048, NULL, 2, NULL);  //Tarefa com prioridade (2) no core 0
  xTaskCreate(&taskMovement, "taskMovement", 2048, NULL, 3, NULL);  //Tarefa com prioridade (3) no core 0
  xTaskCreate(&taskEmergency, "taskEmergency", 2048, NULL, 4, NULL);  //Tarefa com prioridade (4) no core 0
}
