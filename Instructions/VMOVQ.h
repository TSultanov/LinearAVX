#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"

class VMOVQ : public CompilableInstruction<VMOVQ> {
public:
    VMOVQ(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) override {
        movq(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};
