#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

class Transceiver {
    private:
        static const int NSS = 9, IRQ = 5, NRST = 11, BUSY = 10;
        static LR2021 radio;

        volatile static bool transmittedFlag;
        

    public:
        static int init_lr2021();

        static int start_rx_irq();
        static int start_tx_irq();
        static int transmit_irq();
        static int listen_irq();
};

#endif