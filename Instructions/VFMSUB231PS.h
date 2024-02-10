#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VFMSUB231PS : public CompilableInstruction<VFMSUB231PS> {
public:
    VFMSUB231PS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    // Multiply packed single precision floating-point values from xmm2 and xmm3/mem, add to xmm1 and put result in xmm1.
    void implementation(bool upper, bool compile_inline) {
        auto tempReg = getUnusedXmmReg();
        withPreserveXmmReg(tempReg, [=]() {
            movups(xed_reg(tempReg), operands[1].toEncoderOperand(upper));
            mulps(xed_reg(tempReg), operands[2].toEncoderOperand(upper));
            subps(operands[0].toEncoderOperand(upper), xed_reg(tempReg));
        });
        returnReg(tempReg);

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};