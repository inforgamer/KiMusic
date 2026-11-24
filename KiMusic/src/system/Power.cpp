// =============================================================================
//  system/Power.cpp  -  Implementacao (ver Power.hpp)
// =============================================================================
#include "system/Power.hpp"

#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

namespace Power
{

void handleWakeup(uint8_t buttonPin, uint32_t holdMs)
{
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) {
        return;   // ligou a alimentacao ou RESET: liga direto
    }

    // No deep sleep o pino ficou com o RTC; devolve para o GPIO normal.
    rtc_gpio_deinit((gpio_num_t)buttonPin);
    pinMode(buttonPin, INPUT_PULLUP);

    uint32_t start = millis();
    while (millis() - start < holdMs) {
        if (digitalRead(buttonPin) == HIGH) {
            sleep(buttonPin);   // soltou antes do tempo: volta a dormir
        }
        delay(10);
    }
    // Segurou o tempo todo: segue o setup() e liga.
}

void sleep(uint8_t buttonPin)
{
    // Espera soltar o botao (senao o chip acordaria na hora, ainda apertado).
    pinMode(buttonPin, INPUT_PULLUP);
    uint32_t releasedSince = millis();
    while (millis() - releasedSince < 100) {
        if (digitalRead(buttonPin) == LOW) {
            releasedSince = millis();
        }
        delay(5);
    }

    Serial.println("Desligado. Segure o botao central 2 s para ligar.");
    Serial.flush();

    // Acorda quando o botao (ligado ao GND) for para LOW.
    rtc_gpio_pullup_en((gpio_num_t)buttonPin);
    rtc_gpio_pulldown_dis((gpio_num_t)buttonPin);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)buttonPin, 0);
    esp_deep_sleep_start();
}

} // namespace Power
