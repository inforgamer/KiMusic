// =============================================================================
//  system/UsbDisk.hpp  -  Cartao SD como pendrive no PC (USB nativa do ESP32-S3)
// -----------------------------------------------------------------------------
//  O PC le e grava os setores do cartao direto (SD.readRAW/writeRAW). Enquanto
//  isso o player nao pode usar o cartao: os dois escrevendo corrompe o FAT.
//  Por isso run() bloqueia, e quem chama remonta o cartao no fim (o PC pode ter
//  mudado musicas, capas e playlists).
//
//  USB de dados: D- no GPIO 19 e D+ no GPIO 20 (fixos no ESP32-S3).
// =============================================================================
#ifndef SYSTEM_USBDISK_HPP
#define SYSTEM_USBDISK_HPP

#include <stdint.h>

namespace UsbDisk
{
    // Volta quando o PC ejetar o disco, o cabo sair (vbusPin ia HIGH e foi LOW)
    // ou o botao exitPin (ligado ao GND) for apertado.
    void run(uint8_t vbusPin, uint8_t exitPin);
}

#endif
