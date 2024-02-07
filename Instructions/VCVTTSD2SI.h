#include "CompilableInstruction.h"

class VCVTTSD2SI : public CompilableInstruction<VCVTTSD2SI> {
public:
    VCVTTSD2SI(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        cvttsd2si(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};