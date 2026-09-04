#ifndef CHIP_8_PORT_PLATFORM_H
#define CHIP_8_PORT_PLATFORM_H

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ps5Controller.h>

class Platform
{
public:
    Platform(int screenWidth, int screenHeight);

    bool begin();

    void update(
        const void* buffer,
        int pitch
    );

    void processInput(uint8_t* keys);

private:
    TFT_eSPI tft;

    int screenWidth;
    int screenHeight;
};

#endif