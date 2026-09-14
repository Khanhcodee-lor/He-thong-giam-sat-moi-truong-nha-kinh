#include "sensor_payload.h"
#include <assert.h>
#include <math.h>
#include <string.h>
int main(void) {
    uint8_t out[8];
    const uint8_t example[] = {0xBE,0x0A,0x64,0x19,0x90,0x01,0xA8,0x11};
    assert(sensor_payload_encode(27.5,65,400,45.2,out));
    assert(memcmp(out,example,8)==0);
    assert(sensor_payload_encode(-1.25,-1,70000,101,out));
    const uint8_t limits[] = {0x83,0xFF,0,0,0xFF,0xFF,0x10,0x27};
    assert(memcmp(out,limits,8)==0);
    assert(!sensor_payload_encode(NAN,0,0,0,out));
    assert(!sensor_payload_encode(328,0,0,0,out));
    assert(!sensor_payload_encode(0,0,0,0,0));
    assert(sensor_linear_map(2000,3000,1000,0,100)==50);
    assert(sensor_linear_map(2000,1000,3000,0,100)==50);
    assert(isnan(sensor_linear_map(1,2,2,0,100)));
    return 0;
}
