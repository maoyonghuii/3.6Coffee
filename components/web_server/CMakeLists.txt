idf_component_register(SRCS "web_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES dns_server esp_wifi nvs_flash esp_http_server lwip json driver WIFI
                    EMBED_FILES "./www/index.html" 
                    )
