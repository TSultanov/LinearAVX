#include "CompilableInstruction.h"

class VXORPD : public CompilableInstruction<VXORPD> {
public:
    VXORPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() != operands[1].reg()) {
            movups(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper));
        }
        xorpd(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper));
    }
};
