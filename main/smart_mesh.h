#pragma once
#include "esp_err.h"
#include <stdint.h>
esp_err_t smart_mesh_init(void);
esp_err_t smart_mesh_publish(const uint8_t payload[8]);
