#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VUCOMISS : public CompilableInstruction<VUCOMISS> {
public:
    VUCOMISS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        ucomiss(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};