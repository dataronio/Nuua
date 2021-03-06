/**
 * |------------------------|
 * | Nuua Program Structure |
 * |------------------------|
 *
 * Copyright 2019 Erik Campobadal <soc@erik.cat>
 * https://nuua.io
 */
#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "opcodes.hpp"
#include "../../Analyzer/include/analyzer.hpp"
#include <vector>

typedef uint32_t registers_size_t;
typedef enum : uint8_t {
    OT_REG, OT_CONST, OT_GLOBAL, OT_LITERAL, OT_PROP
} OpCodeType;

class Memory;
class Value;

// A frame is the one responsible for storing variables in a program.
class Frame
{
    public:
        // Stores the registers.
        std::unique_ptr<Value[]> registers;
        // Stores the registers size.
        registers_size_t registers_size = 0;
        // Stores the return address to get back to the original program counter.
        opcode_t *return_address = nullptr;
        // Stores the frame caller (the function)
        // Value *caller = nullptr;
        // Allocates the space to store the registers.
        void allocate_registers(registers_size_t size);
        // Frees the allocated register space.
        void free_registers();
        // Setup the frame.
        void setup(registers_size_t size, opcode_t *return_address);
};

// This class is used to represent the frame information during compilation
// (not in runtime).
class FrameInfo
{
    // Stores the registers that are free to use again.
    std::vector<reg_t> free_registers;
    // Stores the registers that are protected to get free unless forçed.
    std::vector<reg_t> protected_registers;
    public:
        // Stores the next register to give in case no free ones are available.
        reg_t current_register = 0;
        // Method to return an available register.
        // It will try to get it from the free_registers
        // otherwise it will return a new register and
        // increment current_register by 1
        reg_t get_register(bool protect = false);
        // Used to free a register.
        void free_register(reg_t reg, bool force = false);
        // Resets the frame info.
        void reset();
};

// The base program class that represents a nuua program.
class Program
{
    public:
        // Stores the main frame of the program.
        Frame main_frame;
        // Stores the program memory (The main code).
        std::unique_ptr<Memory> memory;
        // Constructor to allocate the memory of the
        // program memory. Can't be done here since
        // Memory is a forward declared class.
        Program();
};

// Basic conversation from opcode to string.
std::string opcode_to_string(const opcode_t opcode);
std::vector<OpCodeType> *opcode_operands(const opcode_t opcode);
// Prints a given opcode to the screen.
void print_opcode(const opcode_t opcode);

#endif
