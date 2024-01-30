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

    uint8_t initial_instr[XED_MAX_INSTRUCTION_BYTES];// = { 0xe8, 0x8d, 0x0c, 0x48, 0x00 };
    memcpy(initial_instr, (unsigned char*)info->si_addr, 15);

    xed_decoded_inst_t xedd;
    uint32_t initial_olen;
    decode_instruction2(initial_instr, &xedd, &initial_olen);

    uint8_t buffer[XED_MAX_INSTRUCTION_BYTES];
    unsigned int olen;

    encode_instruction(&xedd, buffer, initial_olen, &olen, tid, uc->uc_mcontext->__ss.__rbp, uc->uc_mcontext->__ss.__rip);

    printf("Initial instruction:\n");
    printf("olen = %d\n", initial_olen);
    for(uint32_t i = 0; i < initial_olen; i++) {
        printf(" %02x", (unsigned int)(*((unsigned char*)info->si_addr + i)));
    }
    printf("\n");

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
    
    memcpy(info->si_addr, buffer, olen);
    if (olen < initial_olen) {
        memset(info->si_addr + (initial_olen - olen), 0x90, initial_olen - olen);
    }
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
    if (signum != SIGILL) {
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
    printf("Recieved SIGTRAP");
}

__attribute__((constructor))
void loadMsg(void)
{
    hello();
    xed_tables_init();
    init_sigill_handler();
}
