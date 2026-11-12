#include <Arduino.h>
#include <stdint.h>

static uint8_t x;

void setup(void)
{
    Serial.begin(115200);
    x = 0;
}

void loop(void)
{
    Serial.printf("%i\n", x);
    ++x;

    delay(1000);
}
