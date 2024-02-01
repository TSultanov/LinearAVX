#include "CompilableInstruction.h"

class VADDPS : public CompilableInstruction<VADDPS> {
public:
    VADDPS(const xed_decoded_inst_t *xedd) : CompilableInstruction(xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        addps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};