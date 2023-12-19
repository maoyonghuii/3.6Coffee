#pragma once

#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif



extern esp_mqtt_client_handle_t client;
void my_mqtt_init();
void send_mqtt_message(esp_mqtt_client_handle_t client, const char* topic, const char* message);





#ifdef __cplusplus
}
#endif