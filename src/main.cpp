#include <Arduino.h>
#include <LittleFS.h>

#include "chip8.h"
#include "Platform.h"

static constexpr int SCREEN_WIDTH = 320;
static constexpr int SCREEN_HEIGHT = 240;

static constexpr const char* PS5_MAC =
    "88:03:4C:78:C2:BB";

static constexpr uint32_t CPU_HZ = 700;
static constexpr uint32_t TIMER_HZ = 60;

static constexpr const char* ROM_FILENAME = "/tetris.ch8";

Platform platform(SCREEN_WIDTH, SCREEN_HEIGHT);
Chip8 chip8;

static uint32_t lastCycleMicros = 0;
static uint32_t cycleAccumulator = 0;
static uint32_t timerAccumulator = 0;
static uint32_t cycleCount = 0;

static bool running = false;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=== CHIP-8 ESP32 ===");

    Serial.println("Starting PS5 controller...");

    if (!ps5.begin(20)) {
        Serial.println("PS5 initialization failed");
    } else {
        Serial.println("PS5 controller scan started");
    }

    // Initialize the TFT first.
    if (!platform.begin())
    {
        Serial.println("TFT initialization failed");
        return;
    }

    // Mount the filesystem
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed");
        return;
    }

    Serial.println("LittleFS OK");

    // Load the CHIP-8 ROM into memory starting at 0x200.
    if (!chip8.LoadRom(ROM_FILENAME))
    {
        Serial.println("ROM load failed");
        return;
    }

    Serial.println("ROM loaded");

    // Draw the initial blank CHIP-8 framebuffer.
    platform.update(
        chip8.video,
        sizeof(chip8.video[0]) * Chip8::VIDEO_WIDTH
    );

    chip8.drawFlag = false;

    lastCycleMicros = micros();
    running = true;

    Serial.println("Starting CHIP-8");
}

void loop()
{
    if (!running)
    {
        delay(100);
        return;
    }

    if (chip8.IsHalted())
    {
        Serial.println("CHIP-8 HALTED");
        running = false;
        return;
    }

    const uint32_t now = micros();
    const uint32_t elapsed = now - lastCycleMicros;
    lastCycleMicros = now;

    uint8_t keys[16]{};

    platform.processInput(keys);

    memcpy(
        chip8.inputKeys,
        keys,
        sizeof(keys)
    );


    cycleAccumulator += elapsed * CPU_HZ;

    const uint32_t cyclesDue =
        cycleAccumulator / 1000000UL;

    cycleAccumulator %= 1000000UL;

    const uint32_t limitedCycles =
        (cyclesDue < 20U) ? cyclesDue : 20U;

    for (uint32_t i = 0; i < limitedCycles; ++i)
    {
        chip8.Cycle();
        ++cycleCount;

        if (chip8.IsHalted())
            break;
    }

    timerAccumulator += elapsed * TIMER_HZ;

    const uint32_t timerTicks =
        timerAccumulator / 1000000UL;

    timerAccumulator %= 1000000UL;

    for (uint32_t i = 0; i < timerTicks; ++i)
    {
        chip8.TickTimers();
    }

    if (chip8.drawFlag)
    {
        platform.update(
            chip8.video,
            sizeof(chip8.video[0]) * Chip8::VIDEO_WIDTH
        );

        chip8.drawFlag = false;
    }

    // Debug approximately once per second.
    if (cycleCount != 0 && cycleCount % CPU_HZ == 0)
    {
        static uint32_t lastPrintedCycle = 0;

        if (cycleCount != lastPrintedCycle)
        {
            Serial.printf(
                "cycles: %lu\n",
                static_cast<unsigned long>(cycleCount)
            );

            lastPrintedCycle = cycleCount;
        }
    }

    yield();
}