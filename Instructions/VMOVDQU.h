#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"

class VMOVDQU : public CompilableInstruction<VMOVDQU> {
public:
    VMOVDQU(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) override {
        movdqu(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};