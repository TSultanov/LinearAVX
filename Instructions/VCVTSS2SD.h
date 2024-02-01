#include "CompilableInstruction.h"

class VCVTSS2SD : public CompilableInstruction<VCVTSS2SD> {
public:
    VCVTSS2SD(const xed_decoded_inst_t *xedd) : CompilableInstruction(xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        cvtss2sd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
    }
};