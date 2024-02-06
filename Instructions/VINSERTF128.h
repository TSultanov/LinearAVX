#include "CompilableInstruction.h"

class VINSERTF128 : public CompilableInstruction<VINSERTF128> {
public:
    VINSERTF128(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        auto value = operands[3].immValue();
        if(value) {
            if (upper) {
                movups(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(false));
            } else {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            }
        } else {
            if (!upper) {
                movups(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(false));
            } else {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            }
        }
    }
};
