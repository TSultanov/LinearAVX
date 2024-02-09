#include "CompilableInstruction.h"

class VHADDPD : public CompilableInstruction<VHADDPD> {
public:
    VHADDPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        map3opto2op(upper, [=](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
            haddpd(op0, op1);
        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};