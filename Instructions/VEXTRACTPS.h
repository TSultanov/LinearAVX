#include "CompilableInstruction.h"

class VEXTRACTPS : public CompilableInstruction<VEXTRACTPS> {
public:
    VEXTRACTPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        extractps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
    }
};
