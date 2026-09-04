#include "Platform.h"

Platform::Platform(int screenWidth, int screenHeight)
    : screenWidth(screenWidth),
      screenHeight(screenHeight)
{
}

bool Platform::begin()
{
    Serial.println("Initializing TFT");

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    Serial.println("TFT init complete");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);

    tft.println("CHIP-8");
    tft.println("Loading Tetris");

    delay(1000);

    tft.fillScreen(TFT_BLACK);

    return true;
}

void Platform::update(
    const void* buffer,
    int pitch
)
{
    if (buffer == nullptr)
        return;

    const uint32_t* video =
        static_cast<const uint32_t*>(buffer);

    constexpr int CHIP8_WIDTH = 64;
    constexpr int CHIP8_HEIGHT = 32;
    constexpr int SCALE = 5;

    const int renderedWidth =
        CHIP8_WIDTH * SCALE;

    const int renderedHeight =
        CHIP8_HEIGHT * SCALE;

    const int xOffset =
        (screenWidth - renderedWidth) / 2;

    const int yOffset =
        (screenHeight - renderedHeight) / 2;

    for (int y = 0; y < CHIP8_HEIGHT; ++y)
    {
        for (int x = 0; x < CHIP8_WIDTH; ++x)
        {
            const bool pixelOn =
                video[y * CHIP8_WIDTH + x] != 0;

            const uint16_t color =
                pixelOn ? TFT_WHITE : TFT_BLACK;

            tft.fillRect(
                xOffset + x * SCALE,
                yOffset + y * SCALE,
                SCALE,
                SCALE,
                color
            );
        }
    }
}

void Platform::processInput(uint8_t* keys)
{
    if (keys == nullptr)
        return;

    memset(keys, 0, 16);

    if (!ps5.isConnected())
        return;

    if (ps5.left)
        keys[0x5] = 1;

    if (ps5.right)
        keys[0x6] = 1;

    if (ps5.down)
        keys[0x7] = 1;

    if (ps5.up)
        keys[0x8] = 1;

    if (ps5.cross)
        keys[0x4] = 1;
}