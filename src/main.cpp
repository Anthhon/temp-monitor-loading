#include <Arduino.h>
#include <esp32-hal-rgb-led.h>
#include <math.h>

#define LED_PIN 48

#define TEMP_OFFSET_FIXED 8.0f
#define TEMP_OFFSET_MULT  3.0f

typedef enum {
    TEMP_UNDEFINED = 0,
    TEMP_LOW,       // <= 10 C
    TEMP_NORMAL,    // <= 45 C
    TEMP_HIGH,      // <= 55 C
    TEMP_CRITICAL,  // > 55 C
} temp_state_t;

typedef enum {
    WHITE  = 0x121212,
    CYAN   = 0x001212,
    GREEN  = 0x001200,
    YELLOW = 0x121200,
    RED    = 0x120000,
    PURPLE = 0x120012,
} color_t;

typedef struct {
    // Temperature info
    float initial_temp_chip;
    float initial_temp_ambient;
    temp_state_t temp_state;

    // LED info
    color_t rgb_color;
} chip_config_t;
static chip_config_t chip;

// Estimate ambient temperature using the initial calibration
static float estimate_ambient_temp(float chip_temp)
{
    float delta = chip_temp - chip.initial_temp_chip;
    return chip.initial_temp_ambient + (delta * TEMP_OFFSET_MULT);
}

// Determine the current temperature state
static temp_state_t get_temp_state(float temp)
{
    if (isnan(temp) || isinf(temp))
        return TEMP_UNDEFINED;

    if (temp <= 10.0f)
        return TEMP_LOW;

    if (temp <= 45.0f)
        return TEMP_NORMAL;

    if (temp <= 55.0f)
        return TEMP_HIGH;

    return TEMP_CRITICAL;
}

// Get the relative RGB color
static color_t get_temp_color(temp_state_t state)
{
    switch (state) {
        case TEMP_LOW:      return CYAN;
        case TEMP_NORMAL:   return GREEN;
        case TEMP_HIGH:     return YELLOW;
        case TEMP_CRITICAL: return RED;
        default:            return PURPLE;
    }
}

// Update the onboard RGB LED
static void update_led(color_t color)
{
    // Using bitwise operations to
    // extract RGB values from color_t
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >>  8) & 0xFF;
    uint8_t b =  color        & 0xFF;

    neopixelWrite(LED_PIN, r, g, b);
}

static float get_chip_temperature(void)
{
    float temp = temperatureRead();
    if (isnan(temp) || isinf(temp)) {

        chip.temp_state = TEMP_UNDEFINED;
        chip.rgb_color = PURPLE;
        update_led(chip.rgb_color);

        return 126.0f; // Temperature sensor has a range of -40ºC to 125ºC
    }

    return temp;
}

void setup(void)
{
    Serial.begin(115200);
    delay(3000);

    // Initialize LED.
    chip.rgb_color = WHITE;
    update_led(chip.rgb_color);

    // Get chip information.
    Serial.println("Informacoes do chip...");
    Serial.printf("Modelo do chip: %s\n", ESP.getChipModel());
    Serial.printf("Flash: %u bytes\n", ESP.getFlashChipSize());
    Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());

    // Initialize temperature sensing.
    Serial.println("Iniciando sensor de temperatura...");

    chip.initial_temp_chip = get_chip_temperature();
    if (chip.initial_temp_chip == 126.0f) {
        Serial.println("Erro ao inicializar sensor!");
        delay(5000);
        return;
    }

    chip.initial_temp_ambient = chip.initial_temp_chip - TEMP_OFFSET_FIXED;
    chip.temp_state = TEMP_NORMAL;
}

void loop(void)
{
    float new_temp_chip = get_chip_temperature();
    if (new_temp_chip == 126.0f) {
        Serial.println("Erro na leitura do sensor!");
        delay(5000);
        return;
    }

    float temp_ambient_estimated = estimate_ambient_temp(new_temp_chip);

    // Update temperature state and LED
    chip.temp_state = get_temp_state(new_temp_chip);
    chip.rgb_color = get_temp_color(chip.temp_state);
    update_led(chip.rgb_color);

    Serial.printf("Temp. Interna:  %.2f C\n", new_temp_chip);
    Serial.printf("Temp. Ambiente: %.2f C (simulado)\n", temp_ambient_estimated);
    Serial.printf("Estado: %d\n", chip.temp_state);

    delay(5000);
}
