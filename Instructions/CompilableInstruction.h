#pragma once
#include "Instruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"
#include "Metadata.h"

template<class T>
class CompilableInstruction : public Instruction {
protected:
    CompilableInstruction(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : Instruction(rip, ilen, xedd) {}

    virtual ~CompilableInstruction() = default;

    void map3opto2op(bool upper, std::function<void(xed_encoder_operand_t const&, xed_encoder_operand_t const&)> instr) {
        if (operands[0].reg() == operands[1].reg()) {
            instr(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                addpd(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                instr(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            instr(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }
    }

    xed_reg_enum_t to32bitGpr(xed_reg_enum_t reg) {
        return (xed_reg_enum_t)(reg - XED_REG_RAX + XED_REG_EAX);
    }

    std::vector<xed_encoder_request_t> const& compile(CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) {
        internal_requests.clear();

        if (compilationStrategy == CompilationStrategy::DirectCall || compilationStrategy == CompilationStrategy::DirectCallPopRax) {
            rspOffset = -8;
        }

        implementation(false, compilationStrategy == CompilationStrategy::Inline);

        if (usesYmm()) {
            with_upper_ymm([=, this]{
                implementation(true, compilationStrategy == CompilationStrategy::Inline);
            });
        }
        
        return internal_requests;
    }

    virtual void implementation(bool upper, bool compile_inline) = 0;
};
