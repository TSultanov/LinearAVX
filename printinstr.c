#include <assert.h>
#include <stdio.h>
#include <xed/xed-interface.h>
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-init.h"

#include "printinstr.h"
#include "utils.h"

void print_memops(xed_decoded_inst_t* xedd) {
    unsigned int i, memops = xed_decoded_inst_number_of_memory_operands(xedd);
 
    debug_print("Memory Operands\n");
    
    for( i=0;i<memops ; i++)   {
        xed_bool_t r_or_w = 0;
        xed_reg_enum_t seg;
        xed_reg_enum_t base;
        xed_reg_enum_t indx;
        debug_print("  %u ",i);
        if ( xed_decoded_inst_mem_read(xedd,i)) {
            debug_print("   read ");
            r_or_w = 1;
        }
        if (xed_decoded_inst_mem_written(xedd,i)) {
            debug_print("written ");
            r_or_w = 1;
        }
        if (!r_or_w) {
            debug_print("   agen "); // LEA instructions
        }
        seg = xed_decoded_inst_get_seg_reg(xedd,i);
        if (seg != XED_REG_INVALID) {
            debug_print("SEG= %s ", xed_reg_enum_t2str(seg));
        }
        base = xed_decoded_inst_get_base_reg(xedd,i);
        if (base != XED_REG_INVALID) {
            debug_print("BASE= %3s/%3s ",
                   xed_reg_enum_t2str(base),
                   xed_reg_class_enum_t2str(xed_reg_class(base)));
        }
        indx = xed_decoded_inst_get_index_reg(xedd,i);
        if (i == 0 && indx != XED_REG_INVALID) {
            debug_print("INDEX= %3s/%3s ",
                   xed_reg_enum_t2str(indx),
                   xed_reg_class_enum_t2str(xed_reg_class(indx)));
            if (xed_decoded_inst_get_scale(xedd,i) != 0) {
                // only have a scale if the index exists.
                debug_print("SCALE= %u ",
                       xed_decoded_inst_get_scale(xedd,i));
            }
        }
 
        if (xed_operand_values_has_memory_displacement(xedd))
        {
            xed_uint_t disp_bits =
                xed_decoded_inst_get_memory_displacement_width(xedd,i);
            if (disp_bits)
            {
                xed_int64_t disp;
                debug_print("DISPLACEMENT_BYTES= %u ", disp_bits);
                disp = xed_decoded_inst_get_memory_displacement(xedd,i);
                debug_print("0x" XED_FMT_LX16 " base10=" XED_FMT_LD, disp, disp);
            }
        }
        debug_print(" ASZ%u=%u\n",
               i,
               xed_decoded_inst_get_memop_address_width(xedd,i));
    }
    debug_print("  MemopBytes = %u\n",
           xed_decoded_inst_get_memory_operand_length(xedd,0));
}

void print_operands(xed_decoded_inst_t* xedd) {
    unsigned int i, noperands;
#define TBUFSZ 90
    char tbuf[TBUFSZ];
    const xed_inst_t* xi = xed_decoded_inst_inst(xedd);
    xed_operand_action_enum_t rw;
    xed_uint_t bits;
    
    debug_print("Operands\n");
    noperands = xed_inst_noperands(xi);
    debug_print("#   TYPE               DETAILS        VIS  RW       OC2 BITS BYTES NELEM ELEMSZ   ELEMTYPE   REGCLASS\n");
    debug_print("#   ====               =======        ===  ==       === ==== ===== ===== ======   ========   ========\n");
    tbuf[0]=0;
    for( i=0; i < noperands ; i++) { 
        const xed_operand_t* op = xed_inst_operand(xi,i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        debug_print("%u %6s ",
               i,xed_operand_enum_t2str(op_name));
 
        switch(op_name) {
          case XED_OPERAND_AGEN:
          case XED_OPERAND_MEM0:
          case XED_OPERAND_MEM1:
            // we print memops in a different function
            xed_strcpy(tbuf, "(see below)");
            break;
          case XED_OPERAND_PTR:     // pointer (always in conjunction with a IMM0)
          case XED_OPERAND_ABSBR:   // Absolute branch displacements
          case XED_OPERAND_RELBR: { // Relative branch displacements
              xed_uint_t disp_bytes =
                  xed_decoded_inst_get_branch_displacement_width(xedd);
              if (disp_bytes) {
              char buf[40];
              const unsigned int no_leading_zeros=0;
              const xed_bool_t lowercase = 1;
              xed_uint_t disp_bits =
                  xed_decoded_inst_get_branch_displacement_width_bits(xedd);
                  xed_int64_t disp =
                      xed_decoded_inst_get_branch_displacement(xedd);
                  xed_itoa_hex_ul(buf, disp, disp_bits, no_leading_zeros, 40, lowercase);
#if defined (_WIN32) && !defined(PIN_CRT)
                  _snprintf_s(tbuf, TBUFSZ, TBUFSZ,
                           "BRANCH_DISPLACEMENT_BYTES=%d 0x%s",
                           disp_bytes,buf);
#else
                  snprintf(tbuf, TBUFSZ,
                           "BRANCH_DISPLACEMENT_BYTES=%d 0x%s",
                           disp_bytes,buf);
#endif
              }
            }
            break;
 
          case XED_OPERAND_IMM0: { // immediates
              char buf[64];
              const unsigned int no_leading_zeros=0;
              xed_uint_t ibits;
              const xed_bool_t lowercase = 1;
              ibits = xed_decoded_inst_get_immediate_width_bits(xedd);
              if (xed_decoded_inst_get_immediate_is_signed(xedd)) {
                  xed_uint_t rbits = ibits?ibits:8;
                  xed_int32_t x = xed_decoded_inst_get_signed_immediate(xedd);
                  xed_uint64_t y = XED_STATIC_CAST(xed_uint64_t,
                                                   xed_sign_extend_arbitrary_to_64(
                                                       (xed_uint64_t)x,
                                                       ibits));
                  xed_itoa_hex_ul(buf, y, rbits, no_leading_zeros, 64, lowercase);
              }
              else {
                  xed_uint64_t x =xed_decoded_inst_get_unsigned_immediate(xedd);
                  xed_uint_t rbits = ibits?ibits:16;
                  xed_itoa_hex_ul(buf, x, rbits, no_leading_zeros, 64, lowercase);
              }
#if defined (_WIN32) && !defined(PIN_CRT)
              _snprintf_s(tbuf, TBUFSZ, TBUFSZ,
                          "0x%s(%db)",buf,ibits);
#else
              snprintf(tbuf,TBUFSZ,
                       "0x%s(%db)",buf,ibits);
#endif
              break;
          }
          case XED_OPERAND_IMM1: { // 2nd immediate is always 1 byte.
              xed_uint8_t x = xed_decoded_inst_get_second_immediate(xedd);
#if defined (_WIN32) && !defined(PIN_CRT)
              _snprintf_s(tbuf, TBUFSZ, TBUFSZ,
                          "0x%02x",(int)x);
#else
              snprintf(tbuf,TBUFSZ,
                       "0x%02x",(int)x);
#endif
 
              break;
          }
 
          case XED_OPERAND_REG0:
          case XED_OPERAND_REG1:
          case XED_OPERAND_REG2:
          case XED_OPERAND_REG3:
          case XED_OPERAND_REG4:
          case XED_OPERAND_REG5:
          case XED_OPERAND_REG6:
          case XED_OPERAND_REG7:
          case XED_OPERAND_REG8:
          case XED_OPERAND_REG9:
          case XED_OPERAND_BASE0:
          case XED_OPERAND_BASE1:
            {
 
              xed_reg_enum_t r = xed_decoded_inst_get_reg(xedd, op_name);
#if defined (_WIN32)  && !defined(PIN_CRT)
              _snprintf_s(tbuf, TBUFSZ, TBUFSZ,
                       "%s=%s", 
                       xed_operand_enum_t2str(op_name),
                       xed_reg_enum_t2str(r));
#else
              snprintf(tbuf,TBUFSZ,
                       "%s=%s", 
                       xed_operand_enum_t2str(op_name),
                       xed_reg_enum_t2str(r));
#endif
              break;
            }
          default: 
            debug_print("need to add support for printing operand: %s",
                   xed_operand_enum_t2str(op_name));
            assert(0);      
        }
        debug_print("%21s", tbuf);
        
        rw = xed_decoded_inst_operand_action(xedd,i);
        
        debug_print(" %10s %3s %9s",
               xed_operand_visibility_enum_t2str(
                   xed_operand_operand_visibility(op)),
               xed_operand_action_enum_t2str(rw),
               xed_operand_width_enum_t2str(xed_operand_width(op)));
 
        bits =  xed_decoded_inst_operand_length_bits(xedd,i);
        debug_print( "  %3u", bits);
        /* rounding, bits might not be a multiple of 8 */
        debug_print("  %4u", (bits +7) >> 3);
 
        debug_print("    %2u", xed_decoded_inst_operand_elements(xedd,i));
        debug_print("    %3u", xed_decoded_inst_operand_element_size_bits(xedd,i));
        
        debug_print(" %10s",
               xed_operand_element_type_enum_t2str(
                   xed_decoded_inst_operand_element_type(xedd,i)));
        debug_print(" %10s\n",
               xed_reg_class_enum_t2str(
                   xed_reg_class(
                       xed_decoded_inst_get_reg(xedd, op_name))));
    }
}


void print_flags(xed_decoded_inst_t* xedd) {
    unsigned int i, nflags;
    if (xed_decoded_inst_uses_rflags(xedd)) {
        const xed_simple_flag_t* rfi = xed_decoded_inst_get_rflags_info(xedd);
        assert(rfi);
        debug_print("FLAGS:\n");
        if (xed_simple_flag_reads_flags(rfi)) {
            debug_print("   reads-rflags ");
        }
        else if (xed_simple_flag_writes_flags(rfi)) {
            //XED provides may-write and must-write information
            if (xed_simple_flag_get_may_write(rfi)) {
                debug_print("  may-write-rflags ");
            }
            if (xed_simple_flag_get_must_write(rfi)) {
                debug_print("  must-write-rflags ");
            }
        }
        nflags = xed_simple_flag_get_nflags(rfi);
        for( i=0;i<nflags ;i++) {
            const xed_flag_action_t* fa =
                xed_simple_flag_get_flag_action(rfi,i);
            char buf[500];
            xed_flag_action_print(fa,buf,500);
            debug_print("%s ", buf);
        }
        debug_print("\n");
        // or as as bit-union
        {
            xed_flag_set_t const*  read_set =
                xed_simple_flag_get_read_flag_set(rfi);
            /* written set include undefined flags */
            xed_flag_set_t const* written_set =
                xed_simple_flag_get_written_flag_set(rfi);
            xed_flag_set_t const* undefined_set =
                xed_simple_flag_get_undefined_flag_set(rfi);
            char buf[500];
            xed_flag_set_print(read_set,buf,500);
            debug_print("       read: %30s mask=0x%x\n",
                   buf,
                   xed_flag_set_mask(read_set));
            xed_flag_set_print(written_set,buf,500);
            debug_print("    written: %30s mask=0x%x\n",
                   buf,
                   xed_flag_set_mask(written_set));
            xed_flag_set_print(undefined_set,buf,500);
            debug_print("  undefined: %30s mask=0x%x\n",
                   buf,
                   xed_flag_set_mask(undefined_set));
        }
#if defined(XED_APX)
        /* print Default Flags Values based on the DFV pseudo register*/
        xed_reg_enum_t dfv_enum = xed_decoded_inst_get_dfv_reg(xedd);
        if (dfv_enum != XED_REG_INVALID){
            xed_flag_dfv_t dfv_reg;
            xed_bool_t okay = xed_flag_dfv_get_default_flags_values(dfv_enum, &dfv_reg);
            assert(okay);
            debug_print("    default:%13sof=%d, sf=%d, zf=%d, cf=%d\n",
                    "",
                    dfv_reg.s.of,
                    dfv_reg.s.sf,
                    dfv_reg.s.zf,
                    dfv_reg.s.cf);
        }
#endif
    }
}

void print_reads_zf_flag(xed_decoded_inst_t* xedd) {
    /* example of reading one bit from the flags set */
    if (xed_decoded_inst_uses_rflags(xedd)) {
        xed_simple_flag_t const* rfi = xed_decoded_inst_get_rflags_info(xedd);
        xed_flag_set_t const* read_set = xed_simple_flag_get_read_flag_set(rfi);
        if (read_set->s.zf) {
            debug_print("READS ZF\n");
        }
    }
}

void print_attributes(xed_decoded_inst_t* xedd) {
    /* Walk the attributes. Generally, you'll know the one you want to
     * query and just access that one directly. */
 
    const xed_inst_t* xi = xed_decoded_inst_inst(xedd);
 
    unsigned int i, nattributes  =  xed_attribute_max();
 
    debug_print("ATTRIBUTES: ");
    for(i=0;i<nattributes;i++) {
        xed_attribute_enum_t attr = xed_attribute(i);
        if (xed_inst_get_attribute(xi,attr))
            debug_print("%s ", xed_attribute_enum_t2str(attr));
    }
    debug_print("\n");
}

void print_instr(xed_decoded_inst_t* xedd) {
    debug_print("iform-enum-name %s\n", 
           xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(xedd)));
    print_operands(xedd);
    print_memops(xedd);
    print_flags(xedd);
    print_reads_zf_flag(xedd);
    print_attributes(xedd);
}