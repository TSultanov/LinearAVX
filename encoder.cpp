#include "encoder.h"
#include "Instructions/Instruction.h"
#include "memmanager.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-error-enum.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-inst.h"
#include "xed/xed-operand-enum.h"
#include "xed/xed-reg-enum.h"
#include <assert.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "Instructions/Instructions.h"

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

/* Common instruction requests */
static xed_encoder_request_t encode_call(uint32_t addr, uint64_t rbp_value) {
  xed_encoder_request_t req;
  xed_encoder_instruction_t enc_inst;

  uint64_t displacement = addr - rbp_value;
  uint32_t displacement32 = (uint32_t)displacement;

  if (displacement != displacement32) {
    printf("Encoding CALL to 0x%08x, RBP: 0x%08llx, displacement64: 0x%08llx, dusplacement32: 0x%04x failed\n", addr, rbp_value, displacement, displacement32);
  }

  auto disp = xed_disp(displacement32, 32);
  xed_inst1(&enc_inst, dstate, XED_ICLASS_CALL_NEAR, 0, xed_mem_bd(XED_REG_RBP, disp, 32));

  xed_convert_to_encoder_request(&req, &enc_inst);
  return req;
}

void encode_instruction(xed_decoded_inst_t *xedd, uint8_t *buffer, const unsigned int ilen, unsigned int *olen, uint64_t tid, uint64_t rbp_value) {
  xed_iclass_enum_t iclass = xed_decoded_inst_get_iclass(xedd);

  if (!iclassMapping.contains(iclass)) {
    printf("Unsupported iclass %s\n", xed_iclass_enum_t2str(iclass));
    exit(1);
  }

  auto instrFactory = iclassMapping.at(iclass);
  std::shared_ptr<Instruction> instr = instrFactory(xedd);

  ymm_t* ymm = get_ymm_for_thread(tid);
  auto requests = instr->compile(ymm);

  uint8_t* chunk = encode_requests(requests);
  
  xed_encoder_request_t req;

  uint64_t chunk_addr = (uint64_t)chunk;

  // TODO handle inline replacement

  encode_call(chunk_addr, rbp_value);
  xed_error_enum_t err = xed_encode(&req, buffer, ilen, olen);
  if (err != XED_ERROR_NONE) {
    printf("Encoder error: %s\n", xed_error_enum_t2str(err));
    exit(1);
  }
}