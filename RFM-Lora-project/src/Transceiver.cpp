#include "Transceiver.h"

int Transceiver::init_lr2021() {
    radio = new Module(NSS, IRQ, NRST, BUSY);
    Serial.begin(9600);

    radio.tcxoVoltage = 3.0;
    radio.irqDioNum = 7;

    return RADIOLIB_ERR_NONE;
}