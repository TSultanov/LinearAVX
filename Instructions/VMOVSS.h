#include "CompilableInstruction.h"

class VMOVSS : public CompilableInstruction<VMOVSS> {
public:
    VMOVSS(const xed_decoded_inst_t *xedd) : CompilableInstruction(xedd) {}
private:
    void implementation(bool upper, bool compile_inline) override {
        assert(operands.size() == 2 || operands.size() == 3);
        if (operands.size() == 2) {
            movss(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        }
        else if (operands.size() == 3) {
            movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            movss(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }
        return;
    }
};
