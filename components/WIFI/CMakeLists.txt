idf_component_register(SRCS "WIFI.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash esp_event web_server UI my_mqtt
                    )
