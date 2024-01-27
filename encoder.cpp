#include "encoder.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode.h"
#include "xed/xed-error-enum.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-inst.h"
#include "xed/xed-operand-enum.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

static void encode_vmovss_xmmdq_memd(xed_encoder_request_t *req, xed_decoded_inst_t *xedd) {
  // Set the machine mode for encoder
  xed_encoder_request_zero_set_mode(req, &dstate);
  // set iclass for encoder
  xed_encoder_request_set_iclass(req, XED_ICLASS_MOVSS);

  const xed_inst_t *xi = xed_decoded_inst_inst(xedd);
  uint32_t noperands = xed_inst_noperands(xi);
  assert(noperands == 2);

  // Decode operand 0
  const xed_operand_t *op0 = xed_inst_operand(xi, 0);
  const xed_operand_enum_t op_name0 = xed_operand_name(op0);
  const xed_reg_enum_t r0 = xed_decoded_inst_get_reg(xedd, op_name0);
  if (r0 < XED_REG_XMM0 && r0 > XED_REG_XMM31) {
    printf("Unsupported register: %s", xed_reg_enum_t2str(r0));
    exit(1);
  }

  xed_encoder_request_set_reg(req, op_name0, r0);
  xed_encoder_request_set_operand_order(req, 0, op_name0);

  // Decode operand 1
  const xed_operand_t *op1 = xed_inst_operand(xi, 1);
  const xed_operand_enum_t op_name1 = xed_operand_name(op1);
  if (op_name1 < XED_OPERAND_MEM0 && op_name1 > XED_OPERAND_MEM1) {
    printf("Unsupported operand: %s", xed_operand_enum_t2str(op_name1));
    exit(1);
  }

  uint32_t memops = xed_decoded_inst_number_of_memory_operands(xedd);
  assert(memops == 1);

  switch (op_name1) {
  case XED_OPERAND_MEM0:
    xed_encoder_request_set_mem0(req);
    xed_encoder_request_set_operand_order(req, 1, XED_OPERAND_MEM0);
    break;
  case XED_OPERAND_MEM1:
    xed_encoder_request_set_operand_order(req, 1, XED_OPERAND_MEM1);
    break;
  default:
    printf("Invalid operand: %s\n", xed_operand_enum_t2str(op_name1));
    exit(1);
  }

  xed_encoder_request_set_effective_operand_width(req, xed_decoded_inst_get_operand_width(xedd));

  xed_encoder_request_set_base0(req, xed_decoded_inst_get_base_reg(xedd,0));
  xed_encoder_request_set_index(req, xed_decoded_inst_get_index_reg(xedd,0));
  xed_encoder_request_set_scale(req, xed_decoded_inst_get_scale(xedd,0));
  xed_encoder_request_set_seg0(req, xed_decoded_inst_get_seg_reg(xedd,0));
  xed_encoder_request_set_memory_operand_length(
                    req,
                    xed_decoded_inst_get_memory_operand_length(xedd,0));
  if (xed_operand_values_has_memory_displacement(xedd)) {
    xed_encoder_request_set_memory_displacement(
                      req,
                      xed_decoded_inst_get_memory_displacement(xedd,0),
                      xed_decoded_inst_get_memory_displacement_width(xedd,0));
  }
  xed3_operand_set_vl(req, xed3_operand_get_vl(xedd));
}

static void encode_vxorps_xmmdq_xmmdq_xmmdq(xed_encoder_request_t *req, xed_decoded_inst_t *xedd) {
  // Set the machine mode for encoder
  xed_encoder_request_zero_set_mode(req, &dstate);
  // set iclass for encoder
  xed_encoder_request_set_iclass(req, XED_ICLASS_XORPS);

  const xed_inst_t *xi = xed_decoded_inst_inst(xedd);
  uint32_t noperands = xed_inst_noperands(xi);
  assert(noperands == 3);

  // Decode operand 0
  const xed_operand_t *op0 = xed_inst_operand(xi, 0);
  const xed_operand_enum_t op_name0 = xed_operand_name(op0);
  const xed_reg_enum_t r0 = xed_decoded_inst_get_reg(xedd, op_name0);
  if (r0 < XED_REG_XMM0 && r0 > XED_REG_XMM31) {
    printf("Unsupported register: %s", xed_reg_enum_t2str(r0));
    exit(1);
  }

  // Decode operand 1
  const xed_operand_t *op1 = xed_inst_operand(xi, 1);
  const xed_operand_enum_t op_name1 = xed_operand_name(op1);
  const xed_reg_enum_t r1 = xed_decoded_inst_get_reg(xedd, op_name1);
  if (r1 < XED_REG_XMM0 && r1 > XED_REG_XMM31) {
    printf("Unsupported register: %s", xed_reg_enum_t2str(r1));
    exit(1);
  }

  // Decode operand 2
  const xed_operand_t *op2 = xed_inst_operand(xi, 2);
  const xed_operand_enum_t op_name2 = xed_operand_name(op2);
  const xed_reg_enum_t r2 = xed_decoded_inst_get_reg(xedd, op_name2);
  if (r2 < XED_REG_XMM0 && r2 > XED_REG_XMM31) {
    printf("Unsupported register: %s", xed_reg_enum_t2str(r1));
    exit(1);
  }

  if (r0 != r1) {
    printf("Having different op0 and op1 is not supported");
    exit(1);
  }

  xed_encoder_request_set_reg(req, op_name0, r0);
  xed_encoder_request_set_operand_order(req, 0, op_name0);

  xed_encoder_request_set_reg(req, op_name1, r2);
  xed_encoder_request_set_operand_order(req, 1, op_name1);

  xed_encoder_request_set_effective_operand_width(req, xed_decoded_inst_get_operand_width(xedd));
  xed3_operand_set_vl(req, xed3_operand_get_vl(xedd));
}

static void encode_vmovups_memqq_ymmqq(xed_encoder_request_t *req, xed_decoded_inst_t *xedd) {
  std::vector<xed_encoder_request_t> internal_requests;

  
}

void encode_instruction(xed_decoded_inst_t *xedd, uint8_t *buffer, const unsigned int ilen, unsigned int *olen, uint64_t tid) {
  xed_iform_enum_t iform = xed_decoded_inst_get_iform_enum(xedd);
  xed_encoder_request_t req;
  switch (iform) {
  case XED_IFORM_VMOVSS_XMMdq_MEMd:
    encode_vmovss_xmmdq_memd(&req, xedd);
    break;
  case XED_IFORM_VXORPS_XMMdq_XMMdq_XMMdq:
    encode_vxorps_xmmdq_xmmdq_xmmdq(&req, xedd);
    break;
  case XED_IFORM_VMOVUPS_MEMqq_YMMqq:
    encode_vmovups_memqq_ymmqq(&req, xedd);
    break;
  default:
    printf("encoder: Unknown instruction");
    exit(1);
  }
  
  xed_error_enum_t err = xed_encode(&req, buffer, ilen, olen);
  if (err != XED_ERROR_NONE) {
    printf("Encoder error: %s\n", xed_error_enum_t2str(err));
    exit(1);
  }
}