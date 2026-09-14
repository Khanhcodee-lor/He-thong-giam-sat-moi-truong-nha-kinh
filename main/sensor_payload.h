#pragma once
#include <stdbool.h>
#include <stdint.h>
bool sensor_payload_encode(float temperature, float humidity, float lux,
                           float soil_percent, uint8_t out[8]);
float sensor_linear_map(int raw, int point_a, int point_b, float value_a, float value_b);
