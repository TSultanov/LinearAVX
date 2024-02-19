#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"

class VPEXTRQ : public CompilableInstruction<VPEXTRQ> {
public:
    VPEXTRQ(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) override {
        if (usesYmm()) {
            debug_print("VPEXTRQ: cannot use YMM\n");
            exit(1);
        }
        pextrq(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
    }
};