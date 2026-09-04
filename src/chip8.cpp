#include <Arduino.h>
#include <LittleFS.h>
#include "chip8.h"

Chip8::Chip8()
    : random_engine(static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count())),
      random_byte(0, 255)
{
    for (uint16_t i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }

    for (auto& fn : table)  fn = &Chip8::OP_NULL;
    for (auto& fn : table0) fn = &Chip8::OP_NULL;
    for (auto& fn : table8) fn = &Chip8::OP_NULL;
    for (auto& fn : tableE) fn = &Chip8::OP_NULL;
    for (auto& fn : tableF) fn = &Chip8::OP_NULL;

    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
}

bool Chip8::LoadRom(const char* filename)
{
    File romfile = LittleFS.open(filename, "r");

    if (!romfile) {
        Serial.printf("FAILED TO OPEN ROM: %s\n", filename);
        return false;
    }

    const size_t fileSize = romfile.size();
    const size_t maxBytes = MEMORY_SIZE - START_ADDRESS;
    const size_t bytesToCopy = (fileSize < maxBytes) ? fileSize : maxBytes;

    memset(memory + START_ADDRESS, 0, maxBytes);

    for (size_t i = 0; i < bytesToCopy; ++i) {
        const int value = romfile.read();
        if (value < 0) {
            romfile.close();
            Serial.println("ERROR READING ROM");
            return false;
        }
        memory[START_ADDRESS + i] = static_cast<uint8_t>(value);
    }

    romfile.close();

    if (fileSize > maxBytes) {
        Serial.printf("ROM too large (%u bytes), truncated to %u bytes\n",
                      static_cast<unsigned int>(fileSize),
                      static_cast<unsigned int>(bytesToCopy));
    }

    pc = START_ADDRESS;
    index = 0;
    sp = 0;
    delayTimer = 0;
    soundTimer = 0;
    halted = false;
    drawFlag = true;

    memset(registers, 0, sizeof(registers));
    memset(stack, 0, sizeof(stack));
    memset(video, 0, sizeof(video));

    Serial.printf("ROM loaded: %u bytes\n", static_cast<unsigned int>(bytesToCopy));
    Serial.printf("First opcode: %02X%02X\n",
                  memory[START_ADDRESS], memory[START_ADDRESS + 1]);

    return true;
}

void Chip8::TickTimers()
{
    if (delayTimer > 0) --delayTimer;
    if (soundTimer > 0) --soundTimer;
}

void Chip8::Cycle()
{
    if (halted) {
        return;
    }

    if (pc >= MEMORY_SIZE - 1) {
        Halt("PC out of bounds");
        return;
    }

    opcode = static_cast<uint16_t>(memory[pc] << 8u) | memory[pc + 1];
    pc += 2;

    const uint8_t highNibble = static_cast<uint8_t>((opcode & 0xF000u) >> 12u);
    ((*this).*(table[highNibble]))();
}

void Chip8::Table0()
{
    if ((opcode & 0x0F00u) != 0) {
        // 0nnn is not required by standard CHIP-8 programs used here.
        return;
    }
    ((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
    ((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
    ((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
    // tableF is 256 entries, so every possible low byte is safe to index.
    ((*this).*(tableF[opcode & 0x00FFu]))();
}

void Chip8::Halt(const char* reason)
{
    halted = true;
    Serial.printf("CHIP-8 HALTED: %s (PC=%04X OPCODE=%04X)\n", reason, pc, opcode);
}

void Chip8::OP_NULL() {}

void Chip8::OP_00E0()
{
    memset(video, 0, sizeof(video));
    drawFlag = true;
}

void Chip8::OP_00EE()
{
    if (sp == 0) {
        Halt("stack underflow");
        return;
    }

    --sp;
    pc = stack[sp];
}

void Chip8::OP_1nnn()
{
    pc = opcode & 0x0FFFu;
}

void Chip8::OP_2nnn()
{
    if (sp >= STACK_SIZE) {
        Halt("stack overflow");
        return;
    }

    stack[sp++] = pc;
    pc = opcode & 0x0FFFu;
}

void Chip8::OP_3xkk()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t byte = opcode & 0x00FFu;
    if (registers[Vx] == byte) pc += 2;
}

void Chip8::OP_4xkk()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t byte = opcode & 0x00FFu;
    if (registers[Vx] != byte) pc += 2;
}

void Chip8::OP_5xy0()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    if (registers[Vx] == registers[Vy]) pc += 2;
}

void Chip8::OP_6xkk()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    registers[Vx] = opcode & 0x00FFu;
}

void Chip8::OP_7xkk()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    registers[Vx] = static_cast<uint8_t>(registers[Vx] + (opcode & 0x00FFu));
}

void Chip8::OP_8xy0()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    registers[Vx] = registers[Vy];
}

void Chip8::OP_8xy1()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    registers[Vx] |= registers[Vy];
}

void Chip8::OP_8xy2()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    registers[Vx] &= registers[Vy];
}

void Chip8::OP_8xy3()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    registers[Vx] ^= registers[Vy];
}

void Chip8::OP_8xy4()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    const uint16_t sum = static_cast<uint16_t>(registers[Vx]) + registers[Vy];

    registers[0xF] = (sum > 0xFF) ? 1 : 0;
    registers[Vx] = static_cast<uint8_t>(sum & 0xFFu);
}

void Chip8::OP_8xy5()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);

    registers[0xF] = (registers[Vx] >= registers[Vy]) ? 1 : 0;
    registers[Vx] = static_cast<uint8_t>(registers[Vx] - registers[Vy]);
}

void Chip8::OP_8xy6()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    registers[0xF] = registers[Vx] & 0x01u;
    registers[Vx] >>= 1;
}

void Chip8::OP_8xy7()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);

    // VF is 1 when Vy >= Vx (no borrow).
    registers[0xF] = (registers[Vy] >= registers[Vx]) ? 1 : 0;
    registers[Vx] = static_cast<uint8_t>(registers[Vy] - registers[Vx]);
}

void Chip8::OP_8xyE()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    registers[0xF] = static_cast<uint8_t>((registers[Vx] & 0x80u) >> 7u);
    registers[Vx] = static_cast<uint8_t>(registers[Vx] << 1u);
}

void Chip8::OP_9xy0()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    if (registers[Vx] != registers[Vy]) pc += 2;
}

void Chip8::OP_Annn()
{
    index = opcode & 0x0FFFu;
}

void Chip8::OP_Bnnn()
{
    const uint16_t target = static_cast<uint16_t>((opcode & 0x0FFFu) + registers[0]);
    if (target >= MEMORY_SIZE) {
        Halt("jump target out of bounds");
        return;
    }
    pc = target;
}

void Chip8::OP_Cxkk()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    registers[Vx] = random_byte(random_engine) & (opcode & 0x00FFu);
}

void Chip8::OP_Dxyn()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t Vy = static_cast<uint8_t>((opcode & 0x00F0u) >> 4u);
    const uint8_t height = opcode & 0x000Fu;

    registers[0xF] = 0;

    for (uint8_t row = 0; row < height; ++row) {
        if (index + row >= MEMORY_SIZE) {
            Halt("sprite memory out of bounds");
            return;
        }

        const uint8_t spriteByte = memory[index + row];
        const uint8_t y = static_cast<uint8_t>((registers[Vy] + row) % VIDEO_HEIGHT);

        for (uint8_t col = 0; col < 8; ++col) {
            if ((spriteByte & (0x80u >> col)) == 0) continue;

            const uint8_t x = static_cast<uint8_t>((registers[Vx] + col) % VIDEO_WIDTH);
            uint32_t& pixel = video[y * VIDEO_WIDTH + x];

            if (pixel != 0) {
                registers[0xF] = 1;
            }

            pixel ^= 0xFFFFFFFFu;
        }
    }

    drawFlag = true;
}

void Chip8::OP_Ex9E()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t key = registers[Vx] & 0x0Fu;
    if (inputKeys[key]) pc += 2;
}

void Chip8::OP_ExA1()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t key = registers[Vx] & 0x0Fu;
    if (!inputKeys[key]) pc += 2;
}

void Chip8::OP_Fx07()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    registers[Vx] = delayTimer;
}

void Chip8::OP_Fx0A()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);

    for (uint8_t key = 0; key < 16; ++key) {
        if (inputKeys[key]) {
            registers[Vx] = key;
            return;
        }
    }
    pc -= 2;
}

void Chip8::OP_Fx15()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    delayTimer = registers[Vx];
}

void Chip8::OP_Fx18()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    soundTimer = registers[Vx];
}

void Chip8::OP_Fx1E()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    index = static_cast<uint16_t>((index + registers[Vx]) & 0x0FFFu);
}

void Chip8::OP_Fx29()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);
    const uint8_t digit = registers[Vx] & 0x0Fu;
    index = static_cast<uint16_t>(FONTSET_START_ADDRESS + (5u * digit));
}

void Chip8::OP_Fx33()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);

    if (index > MEMORY_SIZE - 3) {
        Halt("BCD write out of bounds");
        return;
    }

    uint8_t value = registers[Vx];
    memory[index + 2] = value % 10;
    value /= 10;
    memory[index + 1] = value % 10;
    value /= 10;
    memory[index] = value % 10;
}

void Chip8::OP_Fx55()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);

    if (index + Vx >= MEMORY_SIZE) {
        Halt("register store out of bounds");
        return;
    }

    for (uint8_t i = 0; i <= Vx; ++i) {
        memory[index + i] = registers[i];
    }
}

void Chip8::OP_Fx65()
{
    const uint8_t Vx = static_cast<uint8_t>((opcode & 0x0F00u) >> 8u);

    if (index + Vx >= MEMORY_SIZE) {
        Halt("register load out of bounds");
        return;
    }

    for (uint8_t i = 0; i <= Vx; ++i) {
        registers[i] = memory[index + i];
    }
}