#ifndef MAIASCRIPT_EMULATOR_RV32_HPP
#define MAIASCRIPT_EMULATOR_RV32_HPP

#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <stdexcept>

namespace maiascript {

class EmulatorRV32 {
public:
    uint32_t regs[32];
    uint32_t fregs[32];
    uint32_t pc;
    std::vector<uint8_t> memory;
    bool halted;
    uint64_t instructionCount;

    static const uint32_t MEM_SIZE = 64 * 1024;      // 64 KB memory
    static const uint32_t STACK_TOP = 0x10000 - 16;  // 64 KB top
    static const uint32_t EXIT_TRAP = 0xdeadbeef;

    EmulatorRV32() : pc(0), halted(false), instructionCount(0) {
        memory.resize(MEM_SIZE, 0);
        reset();
    }

    void reset() {
        std::memset(regs, 0, sizeof(regs));
        std::memset(fregs, 0, sizeof(fregs));
        regs[2] = STACK_TOP; // sp
        regs[1] = EXIT_TRAP; // ra
        pc = 0;
        halted = false;
        instructionCount = 0;
    }

    void loadCode(const uint8_t* code, size_t size, uint32_t baseAddr = 0) {
        if (baseAddr + size > MEM_SIZE) {
            throw std::runtime_error("Code exceeds emulator memory size");
        }
        std::memcpy(&memory[baseAddr], code, size);
    }

    int32_t run(uint32_t startPC, uint64_t maxInstructions = 1000000) {
        pc = startPC;
        regs[1] = EXIT_TRAP; // return from main terminates run
        regs[2] = STACK_TOP;
        halted = false;
        instructionCount = 0;

        while (!halted && pc != EXIT_TRAP && instructionCount < maxInstructions) {
            step();
        }

        if (instructionCount >= maxInstructions) {
            throw std::runtime_error("Emulator step limit exceeded (infinite loop)");
        }

        return (int32_t)regs[10]; // return a0
    }

    void step() {
        if (pc + 4 > MEM_SIZE) {
            halted = true;
            return;
        }

        uint32_t instr = memory[pc] | (memory[pc + 1] << 8) |
                         (memory[pc + 2] << 16) | (memory[pc + 3] << 24);
        pc += 4;
        instructionCount++;

        uint32_t opcode = instr & 0x7f;
        uint32_t rd = (instr >> 7) & 0x1f;
        uint32_t funct3 = (instr >> 12) & 0x7;
        uint32_t rs1 = (instr >> 15) & 0x1f;
        uint32_t rs2 = (instr >> 20) & 0x1f;
        uint32_t funct7 = (instr >> 25) & 0x7f;

        auto signExt = [](uint32_t val, int bits) -> int32_t {
            int32_t m = 1U << (bits - 1);
            return (int32_t)((val ^ m) - m);
        };

        regs[0] = 0; // x0 is always hardwired to 0

        switch (opcode) {
            case 0x37: // lui
                if (rd != 0) regs[rd] = instr & 0xfffff000;
                break;
            case 0x17: // auipc
                if (rd != 0) regs[rd] = (pc - 4) + (instr & 0xfffff000);
                break;
            case 0x6f: { // jal
                int32_t imm = signExt(
                    (((instr >> 31) & 1) << 20) |
                    (((instr >> 12) & 0xff) << 12) |
                    (((instr >> 20) & 1) << 11) |
                    (((instr >> 21) & 0x3ff) << 1), 21);
                if (rd != 0) regs[rd] = pc;
                pc = (pc - 4) + imm;
                break;
            }
            case 0x67: { // jalr
                int32_t imm = signExt(instr >> 20, 12);
                uint32_t target = (regs[rs1] + imm) & ~1U;
                if (rd != 0) regs[rd] = pc;
                pc = target;
                break;
            }
            case 0x63: { // Branch
                int32_t imm = signExt(
                    (((instr >> 31) & 1) << 12) |
                    (((instr >> 7) & 1) << 11) |
                    (((instr >> 25) & 0x3f) << 5) |
                    (((instr >> 8) & 0xf) << 1), 13);
                bool take = false;
                switch (funct3) {
                    case 0: take = (regs[rs1] == regs[rs2]); break; // beq
                    case 1: take = (regs[rs1] != regs[rs2]); break; // bne
                    case 4: take = ((int32_t)regs[rs1] < (int32_t)regs[rs2]); break; // blt
                    case 5: take = ((int32_t)regs[rs1] >= (int32_t)regs[rs2]); break; // bge
                    case 6: take = (regs[rs1] < regs[rs2]); break; // bltu
                    case 7: take = (regs[rs1] >= regs[rs2]); break; // bgeu
                }
                if (take) pc = (pc - 4) + imm;
                break;
            }
            case 0x03: { // Load
                int32_t imm = signExt(instr >> 20, 12);
                uint32_t addr = regs[rs1] + imm;
                if (addr >= MEM_SIZE) { halted = true; return; }
                uint32_t val = 0;
                if (funct3 == 0) val = (int32_t)(int8_t)memory[addr]; // lb
                else if (funct3 == 1) val = (int32_t)(int16_t)(memory[addr] | (memory[addr+1] << 8)); // lh
                else if (funct3 == 2) val = memory[addr] | (memory[addr+1] << 8) | (memory[addr+2] << 16) | (memory[addr+3] << 24); // lw
                else if (funct3 == 4) val = memory[addr]; // lbu
                else if (funct3 == 5) val = memory[addr] | (memory[addr+1] << 8); // lhu
                if (rd != 0) regs[rd] = val;
                break;
            }
            case 0x23: { // Store
                int32_t imm = signExt((((instr >> 25) & 0x7f) << 5) | ((instr >> 7) & 0x1f), 12);
                uint32_t addr = regs[rs1] + imm;
                if (addr >= MEM_SIZE) { halted = true; return; }
                if (funct3 == 0) memory[addr] = regs[rs2] & 0xff; // sb
                else if (funct3 == 1) { memory[addr] = regs[rs2] & 0xff; memory[addr+1] = (regs[rs2] >> 8) & 0xff; } // sh
                else if (funct3 == 2) { // sw
                    memory[addr] = regs[rs2] & 0xff;
                    memory[addr+1] = (regs[rs2] >> 8) & 0xff;
                    memory[addr+2] = (regs[rs2] >> 16) & 0xff;
                    memory[addr+3] = (regs[rs2] >> 24) & 0xff;
                }
                break;
            }
            case 0x13: { // OP-IMM
                int32_t imm = signExt(instr >> 20, 12);
                uint32_t shamt = (instr >> 20) & 0x1f;
                uint32_t res = 0;
                switch (funct3) {
                    case 0: res = regs[rs1] + imm; break; // addi
                    case 2: res = ((int32_t)regs[rs1] < imm) ? 1 : 0; break; // slti
                    case 3: res = (regs[rs1] < (uint32_t)imm) ? 1 : 0; break; // sltiu
                    case 4: res = regs[rs1] ^ imm; break; // xori
                    case 6: res = regs[rs1] | imm; break; // ori
                    case 7: res = regs[rs1] & imm; break; // andi
                    case 1: res = regs[rs1] << shamt; break; // slli
                    case 5:
                        if ((funct7 & 0x20) == 0) res = regs[rs1] >> shamt; // srli
                        else res = (int32_t)regs[rs1] >> shamt; // srai
                        break;
                }
                if (rd != 0) regs[rd] = res;
                break;
            }
            case 0x33: { // OP / RV32M
                uint32_t res = 0;
                if (funct7 == 1) { // RV32M
                    switch (funct3) {
                        case 0: res = regs[rs1] * regs[rs2]; break; // mul
                        case 1: res = ((int64_t)(int32_t)regs[rs1] * (int64_t)(int32_t)regs[rs2]) >> 32; break; // mulh
                        case 2: res = ((int64_t)(int32_t)regs[rs1] * (uint64_t)regs[rs2]) >> 32; break; // mulhsu
                        case 3: res = ((uint64_t)regs[rs1] * (uint64_t)regs[rs2]) >> 32; break; // mulhu
                        case 4: res = (regs[rs2] == 0) ? -1 : ((int32_t)regs[rs1] / (int32_t)regs[rs2]); break; // div
                        case 5: res = (regs[rs2] == 0) ? -1 : (regs[rs1] / regs[rs2]); break; // divu
                        case 6: res = (regs[rs2] == 0) ? regs[rs1] : ((int32_t)regs[rs1] % (int32_t)regs[rs2]); break; // rem
                        case 7: res = (regs[rs2] == 0) ? regs[rs1] : (regs[rs1] % regs[rs2]); break; // remu
                    }
                } else {
                    switch (funct3) {
                        case 0:
                            if ((funct7 & 0x20) == 0) res = regs[rs1] + regs[rs2]; // add
                            else res = regs[rs1] - regs[rs2]; // sub
                            break;
                        case 1: res = regs[rs1] << (regs[rs2] & 0x1f); break; // sll
                        case 2: res = ((int32_t)regs[rs1] < (int32_t)regs[rs2]) ? 1 : 0; break; // slt
                        case 3: res = (regs[rs1] < regs[rs2]) ? 1 : 0; break; // sltu
                        case 4: res = regs[rs1] ^ regs[rs2]; break; // xor
                        case 5:
                            if ((funct7 & 0x20) == 0) res = regs[rs1] >> (regs[rs2] & 0x1f); // srl
                            else res = (int32_t)regs[rs1] >> (regs[rs2] & 0x1f); // sra
                            break;
                        case 6: res = regs[rs1] | regs[rs2]; break; // or
                        case 7: res = regs[rs1] & regs[rs2]; break; // and
                    }
                }
                if (rd != 0) regs[rd] = res;
                break;
            }
            case 0x07: { // flw
                int32_t imm = signExt(instr >> 20, 12);
                uint32_t addr = regs[rs1] + imm;
                if (addr + 3 >= MEM_SIZE) { halted = true; return; }
                fregs[rd] = memory[addr] | (memory[addr+1] << 8) | (memory[addr+2] << 16) | (memory[addr+3] << 24);
                break;
            }
            case 0x27: { // fsw
                int32_t imm = signExt((((instr >> 25) & 0x7f) << 5) | ((instr >> 7) & 0x1f), 12);
                uint32_t addr = regs[rs1] + imm;
                if (addr + 3 >= MEM_SIZE) { halted = true; return; }
                memory[addr]   = fregs[rs2] & 0xff;
                memory[addr+1] = (fregs[rs2] >> 8) & 0xff;
                memory[addr+2] = (fregs[rs2] >> 16) & 0xff;
                memory[addr+3] = (fregs[rs2] >> 24) & 0xff;
                break;
            }
            case 0x53: { // OP-FP
                float f1 = 0.0f, f2 = 0.0f;
                std::memcpy(&f1, &fregs[rs1], 4);
                std::memcpy(&f2, &fregs[rs2], 4);
                float f_res = 0.0f;
                switch (funct7) {
                    case 0x00: // fadd.s
                        f_res = f1 + f2;
                        std::memcpy(&fregs[rd], &f_res, 4);
                        break;
                    case 0x04: // fsub.s
                        f_res = f1 - f2;
                        std::memcpy(&fregs[rd], &f_res, 4);
                        break;
                    case 0x08: // fmul.s
                        f_res = f1 * f2;
                        std::memcpy(&fregs[rd], &f_res, 4);
                        break;
                    case 0x0c: // fdiv.s
                        f_res = f1 / f2;
                        std::memcpy(&fregs[rd], &f_res, 4);
                        break;
                    case 0x60: // fcvt.w.s
                        if (rd != 0) regs[rd] = (int32_t)f1;
                        break;
                    case 0x68: // fcvt.s.w
                        f_res = (float)(int32_t)regs[rs1];
                        std::memcpy(&fregs[rd], &f_res, 4);
                        break;
                    case 0x70: // fmv.x.w
                        if (rd != 0) regs[rd] = fregs[rs1];
                        break;
                    case 0x78: // fmv.w.x
                        fregs[rd] = regs[rs1];
                        break;
                    case 0x50: // feq.s, flt.s, fle.s
                        if (funct3 == 2) regs[rd] = (f1 == f2) ? 1 : 0;
                        else if (funct3 == 1) regs[rd] = (f1 < f2) ? 1 : 0;
                        else if (funct3 == 0) regs[rd] = (f1 <= f2) ? 1 : 0;
                        break;
                }
                break;
            }
            default:
                halted = true;
                break;
        }

        regs[0] = 0;
    }
};

} // namespace maiascript

#endif // MAIASCRIPT_EMULATOR_RV32_HPP

