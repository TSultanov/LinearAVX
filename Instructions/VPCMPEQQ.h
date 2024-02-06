#include "CompilableInstruction.h"

class VPCMPEQQ : public CompilableInstruction<VPCMPEQQ> {
public:
    VPCMPEQQ(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() != operands[1].reg()) {
            movups(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper));
        }
        pcmpeqq(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper));

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
