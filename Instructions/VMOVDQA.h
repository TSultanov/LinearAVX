#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"

class VMOVDQA : public CompilableInstruction<VMOVDQA> {
public:
    VMOVDQA(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) override {
        movdqa(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};
