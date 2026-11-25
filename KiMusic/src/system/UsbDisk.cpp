// =============================================================================
//  system/UsbDisk.cpp  -  Implementacao (ver UsbDisk.hpp)
// =============================================================================
#include "system/UsbDisk.hpp"

#include <Arduino.h>
#include <SD.h>
#include <USB.h>
#include <USBMSC.h>

namespace {

constexpr uint32_t SECTOR = 512;

USBMSC msc;
volatile bool ejected = false;

// O TinyUSB pede setores inteiros (offset 0, tamanho multiplo de 512).
int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t size)
{
    uint8_t *p = static_cast<uint8_t *>(buffer);
    if (offset != 0) return -1;
    for (uint32_t i = 0; i < size / SECTOR; i++) {
        if (!SD.readRAW(p + i * SECTOR, lba + i)) return -1;
    }
    return (int32_t)size;
}

int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t size)
{
    if (offset != 0) return -1;
    for (uint32_t i = 0; i < size / SECTOR; i++) {
        if (!SD.writeRAW(buffer + i * SECTOR, lba + i)) return -1;
    }
    return (int32_t)size;
}

bool onStartStop(uint8_t, bool start, bool loadEject)
{
    if (loadEject && !start) {
        ejected = true;   // "Ejetar" no PC
    }
    return true;
}

} // namespace

void UsbDisk::run(uint8_t vbusPin, uint8_t exitPin)
{
    ejected = false;
    msc.vendorID("KiMusic");
    msc.productID("Cartao SD");
    msc.productRevision("1.0");
    msc.onRead(onRead);
    msc.onWrite(onWrite);
    msc.onStartStop(onStartStop);
    msc.mediaPresent(true);
    msc.begin(SD.numSectors(), SECTOR);
    USB.begin();
    Serial.printf("Modo USB: %u setores\r\n", (unsigned)SD.numSectors());

    // Sem o divisor do VBUS (ex.: placa de testes) o pino fica LOW: so sai por
    // "cabo removido" se o cabo chegou a ser visto.
    bool sawCable = false;
    while (!ejected && digitalRead(exitPin) == HIGH) {
        bool cable = digitalRead(vbusPin) == HIGH;
        if (cable) sawCable = true;
        else if (sawCable) break;
        delay(20);
    }
    msc.end();
    while (digitalRead(exitPin) == LOW) {
        delay(10);   // espera soltar: o App nao pode ver esse aperto como Play
    }
    Serial.println(ejected ? "Modo USB: ejetado no PC" : "Modo USB: saiu sem ejetar");
}
