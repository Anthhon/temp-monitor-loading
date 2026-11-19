#include <Arduino.h>
#include <esp32-hal-rgb-led.h>
#include <math.h>
#include <stdint.h>

#define LED_PIN 48

#define TEMP_OFFSET_FIXED 8.0f
#define TEMP_OFFSET_MULT  3.0f

#define TEMP_SENSOR_MINIMUM -40
#define TEMP_SENSOR_MAXIMUM 125

typedef enum {
    GPIO_14 = 14,
    GPIO_10 = 10,
    GPIO_7  = 7,
    GPIO_3  = 3,
} gpio_id;

typedef enum {
    TEMP_UNDEFINED = 0,
    TEMP_LOW,
    TEMP_NORMAL,
    TEMP_HIGH,
    TEMP_CRITICAL,
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
    // Temperature information
    temp_state_t temp_state;
    float initial_temp_chip;
    float initial_temp_ambient;

    // Temperature thresholds
    int threshold_minimum;
    int threshold_average;
    int threshold_maximum;

    // LED information
    color_t rgb_color;
} chip_info_t;

static chip_info_t chip;
static bool get_gpio_touch(gpio_id id);

static float estimate_ambient_temp(float chip_temp)
{
    float delta = chip_temp - chip.initial_temp_chip;

    return chip.initial_temp_ambient +
           (delta * TEMP_OFFSET_MULT);
}

static bool get_chip_temperature(float *temp)
{
    float reading;

    if (temp == NULL)
        return false;

    reading = temperatureRead();

    if (isnan(reading) ||
        isinf(reading) ||
        reading < TEMP_SENSOR_MINIMUM ||
        reading > TEMP_SENSOR_MAXIMUM ||
        get_gpio_touch(GPIO_7)) {
        return false;
    }

    *temp = reading;
    return true;
}

// Determine temperature state
static temp_state_t get_temp_state(float temp)
{
    if (isnan(temp) || isinf(temp))
        return TEMP_UNDEFINED;

    if (temp <= chip.threshold_minimum)
        return TEMP_LOW;

    if (temp <= chip.threshold_average)
        return TEMP_NORMAL;

    if (temp <= chip.threshold_maximum)
        return TEMP_HIGH;

    return TEMP_CRITICAL;
}

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

static void update_led(color_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >>  8) & 0xFF;
    uint8_t b =  color        & 0xFF;

    neopixelWrite(LED_PIN, r, g, b);
}

static bool get_gpio_touch(gpio_id id)
{
    uint32_t reading = touchRead(id);

    switch (id) {
        case GPIO_14: return reading > 24000;
        case GPIO_10: return reading > 38000;
        case GPIO_7:  return reading > 35000;
        case GPIO_3:  return reading > 40000;
        default:      return false;
    }
}

// Get the first pressed configuration button
static bool get_threshold_button(gpio_id* button)
{
    if (button == NULL)
        return false;

    if (get_gpio_touch(GPIO_14)) {
        *button = GPIO_14;
        return true;
    }

    if (get_gpio_touch(GPIO_10)) {
        *button = GPIO_10;
        return true;
    }

    if (get_gpio_touch(GPIO_3)) {
        *button = GPIO_3;
        return true;
    }

    return false;
}

static void print_temperature_thresholds(void)
{
    Serial.printf("- Temperatura baixa <= %d | Temperatura normal <= %d | Temperatura alta <= %d > Temperatura crítica \n",
                  chip.threshold_minimum, chip.threshold_average, chip.threshold_maximum, chip.threshold_maximum);
}

// Increment a threshold while preserving its ordering
static void update_temperature_threshold(gpio_id button)
{
    switch (button) {
        case GPIO_14:
            if (++chip.threshold_minimum >= chip.threshold_average)
                chip.threshold_minimum = TEMP_SENSOR_MINIMUM;
            break;

        case GPIO_10:
            if (++chip.threshold_average >= chip.threshold_maximum)
                chip.threshold_average = chip.threshold_minimum + 1;
            break;

        case GPIO_3:
            if (++chip.threshold_maximum > TEMP_SENSOR_MAXIMUM)
                chip.threshold_maximum = chip.threshold_average + 1;
            break;

        default:
            break;
    }
}

// Configure thresholds until all configuration buttons are released
static void configure_temperature_thresholds(void)
{
    gpio_id button;
    bool modified = false;

    while (get_threshold_button(&button)) {
        if (!modified) { // Avoid printing warning more than once
            Serial.println("[[ MODIFICANDO LIMIARES DE TEMPERATURA ]]");
            modified = true;
        }

        update_temperature_threshold(button);
        print_temperature_thresholds();

        delay(1000);
    }

    if (modified) {
        Serial.println("[[ RETORNANDO AS LEITURAS DE TEMPERATURA ]]");
    }
}

void setup(void)
{
    Serial.begin(115200);
    delay(3000);

    // Initialize LED
    chip.rgb_color = WHITE;
    update_led(chip.rgb_color);

    // Output chip information
    uint32_t flash_size = ESP.getFlashChipSize();
    Serial.println("[[ INFORMAÇÕES DO CHIP ]]");
    Serial.printf("- Modelo do chip: %s\n", ESP.getChipModel());
    Serial.printf("- Flash: %.2fMB (%u bytes)\n", (float)flash_size / (1024.0f * 1024.0f), (unsigned)flash_size);

    // Initialize temperature thresholds
    chip.threshold_minimum = 10;
    chip.threshold_average = 45;
    chip.threshold_maximum = 60;
    Serial.println("[[ LIMIARES DE TEMPERATURA DEFINIDOS ]]");
    print_temperature_thresholds();

    // Initialize temperature sensing
    Serial.println("[[ INICIANDO SENSORIAMENTO DE TEMPERATURA ]]");

    if (!get_chip_temperature(&chip.initial_temp_chip)) {
        Serial.println("Erro ao inicializar sensor!");

        chip.temp_state = TEMP_UNDEFINED;
        chip.rgb_color = PURPLE;
        update_led(chip.rgb_color);

        // Prevent from running with invalid calibration
        while (true) delay(1000);
    }

    // Establish initial calibration
    chip.initial_temp_ambient = chip.initial_temp_chip - TEMP_OFFSET_FIXED;
    chip.temp_state = get_temp_state(chip.initial_temp_ambient);
    chip.rgb_color = get_temp_color(chip.temp_state);

    update_led(chip.rgb_color);
}

void loop(void)
{
    // Configure thresholds before acquiring a new reading
    configure_temperature_thresholds();

    // Acquire current chip temperature
    float new_temp_chip;

    if (!get_chip_temperature(&new_temp_chip)) {
        chip.temp_state = TEMP_UNDEFINED;
        chip.rgb_color = PURPLE;

        update_led(chip.rgb_color);

        Serial.println("Erro na leitura do sensor!");

        delay(1500);
        return;
    }

    // Estimate ambient temperature
    float temp_ambient_estimated = estimate_ambient_temp(new_temp_chip);

    // Update state using estimated ambient temperature
    chip.temp_state = get_temp_state(temp_ambient_estimated);
    chip.rgb_color = get_temp_color(chip.temp_state);
    update_led(chip.rgb_color);

    // Output final information
    Serial.printf( "- Interna: %.2f C | Ambiente: %.2f C (simulado) | Estado: %d\n",
        new_temp_chip, temp_ambient_estimated, chip.temp_state);

    delay(1500);
}
