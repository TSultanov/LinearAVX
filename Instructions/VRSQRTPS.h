#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VRSQRTPS : public CompilableInstruction<VRSQRTPS> {
public:
    VRSQRTPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        rsqrtps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};