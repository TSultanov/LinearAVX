#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VSTMXCSR : public CompilableInstruction<VSTMXCSR> {
public:
    VSTMXCSR(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VSTMXCSR,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_MEM, }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        stmxcsr(operands[0].toEncoderOperand(upper));
    }
};