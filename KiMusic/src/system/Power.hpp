// =============================================================================
//  system/Power.hpp  -  Ligar e desligar pelo botao central
// -----------------------------------------------------------------------------
//  "Desligado" = deep sleep do ESP32-S3 (consumo de microamperes no chip).
//  Quem acorda o chip e o proprio botao central (GPIO 5, pino RTC).
//
//    Ligado    -> Menu > Configuracoes > Desligar -> Device::powerOff() -> Power::sleep()
//    Desligado -> apertar     -> o chip acorda e roda o setup()
//                 -> Power::handleWakeup(): se o botao nao ficar 2 s apertado,
//                    volta a dormir sem ligar a tela (toque acidental)
//
//  Ligar a alimentacao (bateria/USB) ou apertar RESET liga direto, sem os 2 s.
//
//  Atencao: o display, o SD e o DAC continuam alimentados pelo 3V3 durante o
//  deep sleep. Para desligar tudo de verdade (bateria) e preciso uma chave
//  de carga (MOSFET) controlada por um GPIO - ver Documentação/Pinagem.md.
// =============================================================================
#ifndef SYSTEM_POWER_HPP
#define SYSTEM_POWER_HPP

#include <stdint.h>

namespace Power
{
    // Chamar no inicio do setup(), antes de iniciar a tela.
    // Se o chip acordou pelo botao e ele foi solto antes de holdMs, volta a dormir.
    void handleWakeup(uint8_t buttonPin, uint32_t holdMs);

    // Espera o botao ser solto e entra em deep sleep (acorda pelo botao). Nao retorna.
    [[noreturn]] void sleep(uint8_t buttonPin);
}

#endif
