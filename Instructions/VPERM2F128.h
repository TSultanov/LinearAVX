#include "Instruction.h"
#include <unordered_set>

class VPERM2F128 : public Instruction {
public:
    VPERM2F128(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : Instruction(rip, ilen, xedd) {}
private:
    /*
    CASE IMM8[1:0] of
        0: DEST[127:0] := SRC1[127:0]
        1: DEST[127:0] := SRC1[255:128]
        2: DEST[127:0] := SRC2[127:0]
        3: DEST[127:0] := SRC2[255:128]
    ESAC
    CASE IMM8[5:4] of
        0: DEST[255:128] := SRC1[127:0]
        1: DEST[255:128] := SRC1[255:128]
        2: DEST[255:128] := SRC2[127:0]
        3: DEST[255:128] := SRC2[255:128]
    ESAC
    IF (imm8[3])
        DEST[127:0] := 0
    FI
    IF (imm8[7])
        DEST[MAXVL-1:128] := 0
    FI
    */
    std::vector<xed_encoder_request_t> const& compile(CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) {
        internal_requests.clear();

        if (compilationStrategy == CompilationStrategy::DirectCall) {
            rspOffset = -8;
        }

        uint8_t imm8 = operands[3].immValue();
        
        // separate out parts of imm8
        uint8_t a = imm8 & 0b11;
        uint8_t b = (imm8 >> 4) & 0b11;
        uint8_t c = (imm8 >> 3) & 0b1;
        uint8_t d = (imm8 >> 7) & 0b1;

        // take temp Xmm reg
        auto tempReg = getUnusedXmmReg();
        withPreserveXmmReg(tempReg, [=]() {
            if (c) {
                xorps(xed_reg(tempReg), xed_reg(tempReg));
            } else {
                switch(a) {
                    case 0: 
                    {
                        movups(xed_reg(tempReg), operands[1].toEncoderOperand(false));
                        break;
                    }
                    case 1:
                    {
                        std::unordered_set<xed_reg_enum_t> regs = { operands[1].toXmmReg() };
                        swap_in_upper_ymm(regs);
                        movups(xed_reg(tempReg), operands[1].toXmmReg());
                        swap_out_upper_ymm(regs);
                        break;
                    }
                    case 2:
                    {
                        if (operands[2].isMemoryOperand()) {
                            movups(xed_reg(tempReg), operands[2].toEncoderOperand(false));
                        } else {
                            movups(xed_reg(tempReg), operands[2].toXmmReg());
                        }
                        break;
                    }
                    case 3:
                    {
                        if (operands[2].isMemoryOperand()) {
                            movups(xed_reg(tempReg), operands[2].toEncoderOperand(true));
                        } else {
                            std::unordered_set<xed_reg_enum_t> regs = { operands[2].toXmmReg() };
                            swap_in_upper_ymm(regs);
                            movups(xed_reg(tempReg), operands[2].toXmmReg());
                            swap_out_upper_ymm(regs);
                        }
                        break;
                    }
                }
            }

            movups(operands[0].toXmmReg(), xed_reg(tempReg));

            std::unordered_set<xed_reg_enum_t> tempRegs = { tempReg };
            swap_in_upper_ymm(tempRegs);
            if (d) {
                xorps(xed_reg(tempReg), xed_reg(tempReg));
            } else {
                switch(b) {
                    case 0: 
                    {
                        movups(xed_reg(tempReg), operands[1].toEncoderOperand(false));
                        break;
                    }
                    case 1:
                    {
                        std::unordered_set<xed_reg_enum_t> regs = { operands[1].toXmmReg() };
                        swap_in_upper_ymm(regs);
                        movups(xed_reg(tempReg), operands[1].toXmmReg());
                        swap_out_upper_ymm(regs);
                        break;
                    }
                    case 2:
                    {
                        if (operands[2].isMemoryOperand()) {
                            movups(xed_reg(tempReg), operands[2].toEncoderOperand(false));
                        } else {
                            movups(xed_reg(tempReg), operands[2].toXmmReg());
                        }
                        break;
                    }
                    case 3:
                    {
                        if (operands[2].isMemoryOperand()) {
                            movups(xed_reg(tempReg), operands[2].toEncoderOperand(true));
                        } else {
                            std::unordered_set<xed_reg_enum_t> regs = { operands[2].toXmmReg() };
                            swap_in_upper_ymm(regs);
                            movups(xed_reg(tempReg), operands[2].toXmmReg());
                            swap_out_upper_ymm(regs);
                        }
                        break;
                    }
                }
            }
            
            movups(operands[0].toXmmReg(), xed_reg(tempReg));
            swap_out_upper_ymm(tempRegs);
        });
        returnReg(tempReg);
        
        return internal_requests;
    }
};
