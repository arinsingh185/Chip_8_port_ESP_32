#ifndef CHIP_8_PORT_CHIP8_H
#define CHIP_8_PORT_CHIP8_H

#include <cstdint>
#include <random>
#include <chrono>

class Chip8 {
public:
    Chip8();

    bool LoadRom(const char* filename);
    void Cycle();
    void TickTimers();

    uint8_t inputKeys[16]{};

    uint32_t video[64 * 32]{};
    static constexpr uint32_t VIDEO_WIDTH = 64;
    static constexpr uint32_t VIDEO_HEIGHT = 32;

    bool drawFlag{false};

    bool IsHalted() const { return halted; }

private:
    using Chip8Func = void (Chip8::*)();

    void Table0();
    void Table8();
    void TableE();
    void TableF();

    void OP_NULL();
    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();
    void OP_4xkk();
    void OP_5xy0();
    void OP_6xkk();
    void OP_7xkk();
    void OP_8xy0();
    void OP_8xy1();
    void OP_8xy2();
    void OP_8xy3();
    void OP_8xy4();
    void OP_8xy5();
    void OP_8xy6();
    void OP_8xy7();
    void OP_8xyE();
    void OP_9xy0();
    void OP_Annn();
    void OP_Bnnn();
    void OP_Cxkk();
    void OP_Dxyn();
    void OP_Ex9E();
    void OP_ExA1();
    void OP_Fx07();
    void OP_Fx0A();
    void OP_Fx15();
    void OP_Fx18();
    void OP_Fx1E();
    void OP_Fx29();
    void OP_Fx33();
    void OP_Fx55();
    void OP_Fx65();

    void Halt(const char* reason);

    static constexpr uint16_t MEMORY_SIZE = 4096;
    static constexpr uint16_t START_ADDRESS = 0x200;
    static constexpr uint16_t FONTSET_START_ADDRESS = 0x50;
    static constexpr uint16_t STACK_SIZE = 16;
    static constexpr uint16_t FONTSET_SIZE = 80;

    uint8_t registers[16]{};
    uint8_t memory[MEMORY_SIZE]{};
    uint16_t pc{START_ADDRESS};
    uint16_t index{0};
    uint16_t stack[STACK_SIZE]{};
    uint8_t sp{0};
    uint8_t delayTimer{0};
    uint8_t soundTimer{0};
    uint16_t opcode{0};
    bool halted{false};

    std::default_random_engine random_engine;
    std::uniform_int_distribution<uint8_t> random_byte;

    uint8_t fontset[FONTSET_SIZE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    Chip8Func table[16]{};
    Chip8Func table0[16]{};
    Chip8Func table8[16]{};
    Chip8Func tableE[16]{};
    Chip8Func tableF[256]{};
};

#endif // CHIP_8_PORT_CHIP8_H