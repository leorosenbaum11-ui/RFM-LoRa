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

void Transceiver::printStats(void) {
    //print received signal strength index
    Serial.print(F("LR2021 RSSI: "));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    //print signal to noise ratio
    Serial.print(F("LR2021 SNR: "));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));
}

int Transceiver::init_lr2021_lora(int frequency, int bandwidth, int spreadingFactor, int codingRate, int power) {
    radio = new Module(NSS, IRQ, NRST, BUSY);
    Serial.begin(9600);

    transmittedFlag = false;
    receivedFlag = false;
    transmitting = false;
    transmissionState = RADIOLIB_ERR_NONE;

    radio.tcxoVoltage = RadioLibTCXOVoltage_t::Voltage3V0;
    radio.irqDioNum = 7;

    //begin radio and set settings
    Serial.print(F("LR2021 Initializing ... "));
    ConfigLoRa_t config;
    config.frequency = RADIOLIB_UNIT_MEGA(frequency); //recommended 915 Mhz
    config.bandwidth = RADIOLIB_UNIT_KILO(bandwidth); //recommended 125 kHz
    config.spreadingFactor = spreadingFactor; //recommended 9
    config.codingRate = codingRate; //recommended 7
    config.power = power; //recommended 22 dBm

    int state = radio.begin(config);
    if(state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    } 
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        return state;
    }
    radio.setIrqAction(setFlag);

    Transceiver::start_listen_irq();

    return RADIOLIB_ERR_NONE;
}

int Transceiver::start_listen_irq() {
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

int Transceiver::transmit_str(String message) {
    transmitting = true;

    if(!message.length() > 255)
    {
        //transmit string header + message
        transmissionState = radio.startTransmit((char)(strHeader) + message);
    }
    else
    {
        Serial.print("Message too long, 255 characters max");
        return RADIOLIB_ERR_PACKET_TOO_LONG;
    }
    return RADIOLIB_ERR_NONE;
}

int Transceiver::transmit_loc(byte data[]) {
    transmitting = true;

    if(!sizeof(data) > 255)
    {
        //transmit location header + data
        byte out[sizeof(data) + 1];
        memcpy(out + 1, data, sizeof(data));
        out[0] = locHeader;
        
        transmissionState = radio.startTransmit(out, sizeof(out));
    }
    else
    {
        Serial.print("Data too long, 255 bytes max");
        return RADIOLIB_ERR_PACKET_TOO_LONG;
    }
    return RADIOLIB_ERR_NONE;
}

int Transceiver::transmit_sens(byte data[]) {
    transmitting = true;

    if(!sizeof(data) > 255)
    {
        //transmit sensor header + data
        byte out[sizeof(data) + 1];
        memcpy(out + 1, data, sizeof(data));
        out[0] = locHeader;
        
        transmissionState = radio.startTransmit(out, sizeof(out));
    }
    else
    {
        Serial.print("Data too long, 255 bytes max");
        return RADIOLIB_ERR_PACKET_TOO_LONG;
    }
    return RADIOLIB_ERR_NONE;
}

int Transceiver::receive_irq() {
    if(receivedFlag) //check for received data
    {
        receivedFlag = false;

        //read data into byte array
        int numBytes = radio.getPacketLength();
        byte byteArr[numBytes];
        int state = radio.readData(byteArr, numBytes);

        if(state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("LR2021 Received packet! Type: "));
            
            if(byteArr[0] == strHeader) //check for string message
            {
                Serial.println(F("String"));

                //convert rest of byte to string
                int n = sizeof(byteArr);
                char charBuf[n];
                memcpy(charBuf, byteArr + 1, n - 1);
                charBuf[n - 1] = '\0';

                String temp = String(charBuf);
                strBuffer.push_back(std::move(temp));
                
                Transceiver::printStats();
            }
            else if (byteArr[0] == locHeader) //check for location data
            {
                Serial.println(F("Location data"));

                locBuffer.insert(locBuffer.end(), byteArr, byteArr + sizeof(byteArr));

                Transceiver::printStats();
            }
            else if(byteArr[0] == sensHeader) //check for sensor data
            {
                Serial.println(F("Sensor data"));
                
                sensBuffer.insert(sensBuffer.end(), byteArr, byteArr + sizeof(byteArr));

                Transceiver::printStats();
            }
        }
        else if (state == RADIOLIB_ERR_CRC_MISMATCH)
        {
            Serial.println(F("CRC error! Packet malformed!"));
            
            return RADIOLIB_ERR_CRC_MISMATCH;
        }
        else
        {
            Serial.print(F("Failed, code "));
            Serial.println(state);
            return state;
        }
    }
    return RADIOLIB_ERR_NONE;
}

int Transceiver::clean_irq() {
    if(transmittedFlag) //check for finished transmission
    {
        //reset flags
        transmittedFlag = false;
        transmitting = false;

        if (transmissionState == RADIOLIB_ERR_NONE)
        {
            Serial.println("Transmission finished!");
        }
        else
        {
            Serial.print("Failed, code ");
            Serial.println(transmissionState);
            return transmissionState;
        }

        radio.finishTransmit(); //clean up after transmission

        return RADIOLIB_ERR_NONE;
    }
}