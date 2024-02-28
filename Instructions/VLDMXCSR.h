#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VLDMXCSR : public CompilableInstruction<VLDMXCSR> {
public:
    VLDMXCSR(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VLDMXCSR,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_MEM, }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        ldmxcsr(operands[0].toEncoderOperand(upper));
    }
};