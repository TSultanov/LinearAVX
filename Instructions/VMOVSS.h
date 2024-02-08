#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"
#include <cstdio>
#include <unistd.h>

class VMOVSS : public CompilableInstruction<VMOVSS> {
public:
    VMOVSS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) override {
        assert(operands.size() == 2 || operands.size() == 3);
        if (operands.size() == 2) {
            movss(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        }
        else if (operands.size() == 3) {
            if (operands[0].reg() == operands[1].reg()) {
                movss(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
            } else if (operands[0].reg() == operands[2].reg()) {
                withPreserveXmmReg(operands[1], [=]() {
                    movss(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                    movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
                });
            } else {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
                movss(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
            }
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
