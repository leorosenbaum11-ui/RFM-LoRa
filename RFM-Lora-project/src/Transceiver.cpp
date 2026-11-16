#include "Transceiver.h"

void Transceiver::setFlag(void) {
    if(transmitting == false)
    {
        receivedFlag = true;
    }
    else
    {
        transmittedFlag = true;
    }
}

int Transceiver::init_lr2021_lora() {
    radio = new Module(NSS, IRQ, NRST, BUSY);
    Serial.begin(9600);

    transmittedFlag = false;
    receivedFlag = false;
    transmitting = false;

    radio.tcxoVoltage = RadioLibTCXOVoltage_t::Voltage3V0;
    radio.irqDioNum = 7;

    //begin radio and set settings
    Serial.print(F("LR2021 Initializing ... "));
    ConfigLoRa_t config;
    config.frequency = RADIOLIB_UNIT_MEGA(frequency);
    config.bandwidth = RADIOLIB_UNIT_KILO(bandwidth);
    config.spreadingFactor = spreadingFactor;
    config.codingRate = codingRate;
    config.power = power;

    int state = radio.begin(config);
    if(state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        return state;
    }
    radio.setIrqAction(setFlag);

    Transceiver::listen_irq();

    return RADIOLIB_ERR_NONE;
}

int Transceiver::listen_irq() {
    Serial.print(F("LR2021 Starting to listen ... "));
    int state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        return state;
    }
    return RADIOLIB_ERR_NONE;
}

int Transceiver::receive_irq() {
    if(receivedFlag)
    {
        receivedFlag = false;

    }
}