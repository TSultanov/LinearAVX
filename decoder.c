#include "decoder.h"
#include "printinstr.h"
#include "xed/xed-address-width-enum.h"
#include "xed/xed-decode.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-machine-mode-enum.h"
#include <stdio.h>

void decode_instruction(unsigned char *inst, xed_decoded_inst_t *xedd, uint32_t *olen) {
    xed_machine_mode_enum_t mmode = XED_MACHINE_MODE_LONG_64;
    xed_address_width_enum_t stack_addr_width = XED_ADDRESS_WIDTH_64b;

    const uint32_t instruction_length = 15;

    xed_error_enum_t xed_error;
    xed_decoded_inst_zero(xedd);
    xed_decoded_inst_set_mode(xedd, mmode, stack_addr_width);
    xed_error = xed_decode(xedd, 
                            XED_STATIC_CAST(const xed_uint8_t*,inst),
                            instruction_length);
    *olen = xed_decoded_inst_get_length(xedd);
    printf("Length: %d, Error: %s\n",(int)*olen, xed_error_enum_t2str(xed_error));
    print_instr(xedd);
}