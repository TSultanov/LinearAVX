#include "CompilableInstruction.h"

class VCVTTSS2SI : public CompilableInstruction<VCVTTSS2SI> {
public:
    VCVTTSS2SI(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        cvttss2si(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};