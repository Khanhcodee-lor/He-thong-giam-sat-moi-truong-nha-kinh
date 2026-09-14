#include "sensor_payload.h"
#include <math.h>
#include <stddef.h>
static float clamp(float x, float lo, float hi) { return fminf(hi, fmaxf(lo, x)); }
static void put_le16(uint8_t *p, uint16_t x) { p[0] = x & 0xff; p[1] = x >> 8; }
float sensor_linear_map(int raw, int a, int b, float va, float vb)
{
    if (a == b) return NAN;
    return va + (float)(raw - a) * (vb - va) / (float)(b - a);
}
bool sensor_payload_encode(float t, float h, float lux, float soil, uint8_t out[8])
{
    if (!out || !isfinite(t) || !isfinite(h) || !isfinite(lux) || !isfinite(soil) ||
        t < -327.68f || t > 327.67f) return false;
    put_le16(out, (uint16_t)(int16_t)lroundf(t * 100));
    put_le16(out + 2, (uint16_t)lroundf(clamp(h, 0, 100) * 100));
    put_le16(out + 4, (uint16_t)lroundf(clamp(lux, 0, 65535)));
    put_le16(out + 6, (uint16_t)lroundf(clamp(soil, 0, 100) * 100));
    return true;
}
