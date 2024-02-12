#include "CompilableInstruction.h"

class VCOMISS : public CompilableInstruction<VCOMISS> {
public:
    VCOMISS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        comiss(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};