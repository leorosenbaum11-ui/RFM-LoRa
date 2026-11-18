#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <vector>

class Transceiver {
    private:
        static const int NSS = 9, IRQ = 5, NRST = 11, BUSY = 10;
        static LR2021 radio;

        static const uint8_t strHeader = 0x3D, locHeader = 0x4E, sensHeader = 0x5F;

        //flags
        volatile static bool transmittedFlag;
        volatile static bool receivedFlag;
        volatile static bool transmitting;

        static int transmissionState;

        //received data buffers
        static std::vector<String> strBuffer;
        static std::vector<byte> locBuffer;
        static std::vector<byte> sensBuffer;

    public:
        #if defined(ESP8266) || defined(ESP32)
            ICACHE_RAM_ATTR
        #endif
        static void setFlag(void);
        static void printStats(void);

        static int init_lr2021_lora(int frequency, int bandwidth, int spreadingFactor, int codingRate, int power); //set up radio

        static int start_listen_irq(); //start listening
        static int transmit_str(String message); //transmit string message packet
        static int transmit_loc(byte data[]); //transmit location data packet
        static int transmit_sens(byte data[]); //transmit sensor data packet
        static int receive_irq(); //receive check, place in loop, auto checks for data types
        static int clean_irq(); //clean up finished transmission, place in loop
};

#endif