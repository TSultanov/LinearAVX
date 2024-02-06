#include "CompilableInstruction.h"

class VMOVAPS : public CompilableInstruction<VMOVAPS> {
public:
    VMOVAPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        movaps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};