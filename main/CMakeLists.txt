idf_component_register(SRCS "main.c" "smart_mesh.c" "sensor_payload.c"
                            "src/cambienquangtro/quangtro.c"
                            "src/doamdat/doamdat.c"
                            "src/mucnuoc/mucnuoc.c"
                            "src/dht11/dht11.c"
                            "src/mq2/mq2.c"
                    INCLUDE_DIRS "."
                                 "src/cambienquangtro"
                                 "src/doamdat"
                                 "src/mucnuoc"
                                 "src/dht11"
                                 "src/mq2"
                    REQUIRES bt nvs_flash esp_hw_support esp_adc
                             driver
                             esp_timer)
