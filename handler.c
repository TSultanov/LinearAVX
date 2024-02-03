#include <assert.h>
#include <mach/kern_return.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <mach/vm_map.h>
#include <signal.h>
#include <sys/signal.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <unistd.h>
#include "handler.h"
#include "encoder.h"
#include <libproc.h>
#include <pthread.h>
#include "memmanager.h"
#include "printinstr.h"
#include "decoder.h"

void hello(void)
{
    printf("Avxhandler loaded\n");
    struct proc_bsdshortinfo info;
    pid_t pid = getpid(); // Get the PID of the current process

    if (proc_pidinfo(pid, PROC_PIDT_SHORTBSDINFO, 0, &info, sizeof(info)) > 0) {
        printf("Current process name: %s\n", info.pbsi_comm);
    } else {
        printf("Failed to get process name.\n");
    }
}

void decode_instruction2(unsigned char *inst, xed_decoded_inst_t *xedd, uint32_t *olen) {
    xed_machine_mode_enum_t mmode = XED_MACHINE_MODE_LONG_64;
    xed_address_width_enum_t stack_addr_width = XED_ADDRESS_WIDTH_64b;

    uint32_t instruction_length = 15;

    xed_error_enum_t xed_error;
    xed_decoded_inst_zero(xedd);
    xed_decoded_inst_set_mode(xedd, mmode, stack_addr_width);
    xed_error = xed_decode(xedd, 
                            XED_STATIC_CAST(const xed_uint8_t*,inst),
                            instruction_length);
    printf("Length: %d, Error: %s\n",(int)xed_decoded_inst_get_length(xedd), xed_error_enum_t2str(xed_error));
    *olen = xed_decoded_inst_get_length(xedd);
    print_instr(xedd);
}

void sigill_handler(int sig, siginfo_t *info, void *ucontext) {
    ucontext_t *uc = (ucontext_t *)ucontext;
    printf("\n========================\n");
    uint64_t tid;
    pthread_t self;
    self = pthread_self();
    pthread_threadid_np(self, &tid);

    printf("Invalid instruction at %p in thread %p [%llu]\n", info->si_addr, self, tid);
    printf("RIP: %llx\n", uc->uc_mcontext->__ss.__rip);
    printf("RSP: %llx\n", uc->uc_mcontext->__ss.__rsp);
    printf("RBP: %llx\n", uc->uc_mcontext->__ss.__rbp);

    // // print GPR
    printf("RAX: %llx\n", uc->uc_mcontext->__ss.__rax);
    printf("RBX: %llx\n", uc->uc_mcontext->__ss.__rbx);
    printf("RCX: %llx\n", uc->uc_mcontext->__ss.__rcx);
    printf("RDX: %llx\n", uc->uc_mcontext->__ss.__rdx);
    printf("RSI: %llx\n", uc->uc_mcontext->__ss.__rsi);
    printf("RDI: %llx\n", uc->uc_mcontext->__ss.__rdi);
    printf("R8: %llx\n", uc->uc_mcontext->__ss.__r8);
    printf("R9: %llx\n", uc->uc_mcontext->__ss.__r9);
    printf("R10: %llx\n", uc->uc_mcontext->__ss.__r10);
    printf("R11: %llx\n", uc->uc_mcontext->__ss.__r11);
    printf("R12: %llx\n", uc->uc_mcontext->__ss.__r12);
    printf("R13: %llx\n", uc->uc_mcontext->__ss.__r13);
    printf("R14: %llx\n", uc->uc_mcontext->__ss.__r14);
    printf("R15: %llx\n", uc->uc_mcontext->__ss.__r15);

    // print memory at RAX for 16 bytes
    // if (uc->uc_mcontext->__ss.__rip == 0x140001620) {
    //     printf("Memory at RAX:\n");
    //     uint8_t membuff[32];
    //     memcpy(membuff, (unsigned char*)uc->uc_mcontext->__ss.__rax, 32);
    //     for (int i = 0; i < 32; i++) {
    //         printf("%01x ", membuff[i]);
    //     }
    //     printf("\n");
    // }

    // print XMM
    ymm_t* ymm_state = get_ymm_for_thread(tid);

    uint64_t buff[2];
    memcpy(buff, &ymm_state->u[0], sizeof(__m128));
    printf("YMM0: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm0, sizeof(uc->uc_mcontext->__fs.__fpu_xmm0));
    printf("XMM0: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[1], sizeof(__m128));
    printf("YMM1: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm1, sizeof(uc->uc_mcontext->__fs.__fpu_xmm1));
    printf("XMM1: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[2], sizeof(__m128));
    printf("YMM2: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm2, sizeof(uc->uc_mcontext->__fs.__fpu_xmm2));
    printf("XMM2: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[3], sizeof(__m128));
    printf("YMM3: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm3, sizeof(uc->uc_mcontext->__fs.__fpu_xmm3));
    printf("XMM3: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[4], sizeof(__m128));
    printf("YMM4: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm4, sizeof(uc->uc_mcontext->__fs.__fpu_xmm4));
    printf("XMM4: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[5], sizeof(__m128));
    printf("YMM5: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm5, sizeof(uc->uc_mcontext->__fs.__fpu_xmm5));
    printf("XMM5: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[6], sizeof(__m128));
    printf("YMM6: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm6, sizeof(uc->uc_mcontext->__fs.__fpu_xmm6));
    printf("XMM6: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[7], sizeof(__m128));
    printf("YMM7: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm7, sizeof(uc->uc_mcontext->__fs.__fpu_xmm7));
    printf("XMM7: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[8], sizeof(__m128));
    printf("YMM8: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm7, sizeof(uc->uc_mcontext->__fs.__fpu_xmm8));
    printf("XMM8: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[9], sizeof(__m128));
    printf("YMM9: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm9, sizeof(uc->uc_mcontext->__fs.__fpu_xmm9));
    printf("XMM9: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[10], sizeof(__m128));
    printf("YMM10: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm10, sizeof(uc->uc_mcontext->__fs.__fpu_xmm10));
    printf("XMM10: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[11], sizeof(__m128));
    printf("YMM11: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm11, sizeof(uc->uc_mcontext->__fs.__fpu_xmm11));
    printf("XMM11: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[12], sizeof(__m128));
    printf("YMM12: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm12, sizeof(uc->uc_mcontext->__fs.__fpu_xmm12));
    printf("XMM12: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[13], sizeof(__m128));
    printf("YMM13: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm13, sizeof(uc->uc_mcontext->__fs.__fpu_xmm13));
    printf("XMM13: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[14], sizeof(__m128));
    printf("YMM14: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm14, sizeof(uc->uc_mcontext->__fs.__fpu_xmm14));
    printf("XMM14: %llx %llx\n", buff[0], buff[1]);

    memcpy(buff, &ymm_state->u[15], sizeof(__m128));
    printf("YMM15: %llx %llx ", buff[0], buff[1]);
    memcpy(buff, &uc->uc_mcontext->__fs.__fpu_xmm15, sizeof(uc->uc_mcontext->__fs.__fpu_xmm15));
    printf("XMM15: %llx %llx\n", buff[0], buff[1]);

    uint8_t initial_instr[XED_MAX_INSTRUCTION_BYTES];// = { 0xe8, 0x8d, 0x0c, 0x48, 0x00 };
    memcpy(initial_instr, (unsigned char*)info->si_addr, 15);

    xed_decoded_inst_t xedd;
    uint32_t initial_olen;
    decode_instruction2(initial_instr, &xedd, &initial_olen);

    printf("Initial instruction:\n");
    printf("olen = %d\n", initial_olen);
    for(uint32_t i = 0; i < initial_olen; i++) {
        printf(" %02x", (unsigned int)(*((unsigned char*)info->si_addr + i)));
    }
    printf("\n");

    uint8_t buffer[XED_MAX_INSTRUCTION_BYTES];
    unsigned int olen;

    encode_instruction(&xedd, buffer, initial_olen, &olen, tid, uc->uc_mcontext->__ss.__rbp, uc->uc_mcontext->__ss.__rip, uc->uc_mcontext->__ss.__rsp);

    printf("Translated instruction:\n");
    printf("olen = %d\n", olen);
    for(uint32_t i = 0; i < olen; i++) {
        printf(" %02x", (unsigned int)buffer[i]);
    }
    printf("\n");
    xed_decoded_inst_t xedd2;
    uint32_t initial_olen2;
    decode_instruction2(buffer, &xedd2, &initial_olen2);

    if (olen > initial_olen) {
        printf("Translated instruction is longer than original one (%d > %d), cannot continue\n", olen, initial_olen);
        exit(1);
    }

    printf("Enable write flag on memory\n");

    int pid;
    mach_port_t task;
    // kern_return_t kret = task_for_pid(mach_task_self(), pid, &task);
    // if (kret != KERN_SUCCESS) {
    //     printf("task_for_pid failed: %d\n", kret);
    //     exit(1);
    // }
    kern_return_t kret = vm_protect(current_task(), (vm_address_t)info->si_addr, initial_olen, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_ALL);
    if (kret != KERN_SUCCESS) {
        printf("vm_protect failed: %d\n", kret);
        exit(1);
    }
    printf("Copying instruction...\n");
    
    printf("Aligining with NOPs... for %d bytes\n", initial_olen - olen);
    if (olen < initial_olen) {
        memset(info->si_addr/* + (initial_olen - olen)*/, 0x90, initial_olen - olen);
    }
    memcpy(info->si_addr + (initial_olen - olen), buffer, olen);
    printf("Copy OK\n");
    // kret = vm_protect(task, (vm_address_t)info->si_addr, initial_olen, FALSE, VM_PROT_READ | VM_PROT_EXECUTE | VM_PROT_ALL);
    // if (kret != KERN_SUCCESS) {
    //     printf("vm_protect failed: %d\n", kret);
    //     exit(1);
    // }
}

#define DYLD_INTERPOSE(_replacment,_replacee) \
  __attribute__((used)) static struct{ const void* replacment; const void* replacee; } _interpose_##_replacee \
  __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, (const void*)(unsigned long)    &_replacee };

int	mysigaction(int signum, const struct sigaction * __restrict act, struct sigaction * __restrict oldact) {
    if (signum != SIGILL && signum != SIGTRAP) {
        printf("sigaction: Installing handler for signal %d\n", signum);
        return sigaction(signum, act, oldact);
    }

    return 0;
}

DYLD_INTERPOSE(mysigaction, sigaction);

void init_sigill_handler(void) {
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_sigaction = &sigill_handler;
    act.sa_flags = SA_SIGINFO;
    int res = sigaction(SIGILL, &act, NULL);
    if (res < 0) {
        perror("sigaction");
        exit(1);
    }
}

void sigtrap_handler(int sig, siginfo_t *info, void *ucontext) {
    uint64_t rip = ((ucontext_t*)ucontext)->uc_mcontext->__ss.__rip;
    void* chunk = jumptable_get_chunk(rip-1); // RIP points to instruction after the trap instruction
    if (chunk == NULL) {
        printf("sigtrap_handler: No chunk found for rip 0x%llx\n", rip);
        exit(1);
    }

    printf("sigtrap_handler: Found chunk for rip 0x%llx at %p\n", rip, chunk);
    // Save return address on stack
    uint64_t rsp = ((ucontext_t*)ucontext)->uc_mcontext->__ss.__rsp - 8;
    uint64_t ret_addr = rip;
    *((uint64_t*)(rsp)) = ret_addr;
    ((ucontext_t*)ucontext)->uc_mcontext->__ss.__rsp = rsp;
    printf("sigtrap_handler: Saved return address 0x%llx on stack\n", ret_addr);

    // Set RIP to point to chunk start
    ((ucontext_t*)ucontext)->uc_mcontext->__ss.__rip = (uint64_t)chunk;
}

void init_sigtrap_handler(void) {
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_sigaction = &sigtrap_handler;
    act.sa_flags = SA_SIGINFO;
    int res = sigaction(SIGTRAP, &act, NULL);
    if (res < 0) {
        perror("sigaction");
        exit(1);
    }
}

__attribute__((constructor))
void loadMsg(void)
{
    hello();
    xed_tables_init();
    init_sigill_handler();
    init_sigtrap_handler();
}
