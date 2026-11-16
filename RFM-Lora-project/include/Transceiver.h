#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

class Transceiver {
    private:
        static const int NSS = 9, IRQ = 5, NRST = 11, BUSY = 10;
        static LR2021 radio;

        static int frequency; //915 MHz
        static int bandwidth; //125 kHz
        static int spreadingFactor; //9
        static int codingRate; //7
        static int power; //22 dBm

        volatile static bool transmittedFlag;
        volatile static bool receivedFlag;
        volatile static bool transmitting;
        

    public:
        #if defined(ESP8266) || defined(ESP32)
            ICACHE_RAM_ATTR
        #endif
        static void setFlag(void);
    
        static int init_lr2021_lora();

        static int transmit_irq(); //set up radio
        static int listen_irq(); //start listening
        static int receive_irq(); //interrupt check, place in loop
};

#endif