#include "CompilableInstruction.h"

class VBLENDVPD : public CompilableInstruction<VBLENDVPD> {
public:
    VBLENDVPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        for (auto& op : operands) {
            if (op.reg() == XED_REG_XMM0) {
                printf("ERROR: VBLENDPVD with XMM0 is not supported (yet)\n");
                exit(1);
            }
        }

        if (operands[0].reg() != operands[1].reg()) {
            movups(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper));
        }
        withPreserveXmmReg(XED_REG_XMM0, [=]() {
            movups(xed_reg(XED_REG_XMM0), operands[3].toEncoderOperand(upper));
            blendvpd(
                operands[0].toEncoderOperand(upper),
                operands[2].toEncoderOperand(upper));

        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
