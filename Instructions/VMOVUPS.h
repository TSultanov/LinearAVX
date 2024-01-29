#include "CompilableInstruction.h"

class VMOVUPS : public CompilableInstruction<VMOVUPS> {
public:
    VMOVUPS(const xed_decoded_inst_t *xedd) : CompilableInstruction(xedd) {}
private:
    void implementation(bool upper) {
        movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};