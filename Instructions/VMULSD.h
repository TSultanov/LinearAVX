#include "CompilableInstruction.h"

class VMULSD : public CompilableInstruction<VMULSD> {
public:
    VMULSD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        map3opto2op(upper, [=](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
            mulsd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};