#include <signal.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include "handler.h"

void hello(void)
{
    printf("Avxhandler loaded\n");
}

#define VEX3_PREFIX 0xC4
#define VEX2_PREFIX 0xC5

#define VEX2_R (1<<7)
#define VEX2_v3 (1<<6)
#define VEX2_v2 (1<<5)
#define VEX2_v1 (1<<4)
#define VEX2_v0 (1<<3)
#define VEX2_L (1<<2)
#define VEX2_p1 (1<<1)
#define VEX2_p0 (1<<0)

const char* vex_prefix[4] = {
    "packed single",
    "packed double",
    "scalar single",
    "scalar double",
};

void decode_instruction(unsigned char *inst) {
    switch(*inst) {
        case VEX2_PREFIX:
            printf("Instruction prefix: VEX2 (%02x)\n", *inst);
            inst++;
            // print out bits in instruction
            printf("R: %d\n", (*inst & VEX2_R)!= 0);
            printf("v3: %d\n", (*inst & VEX2_v3)!= 0);
            printf("v2: %d\n", (*inst & VEX2_v2)!= 0);
            printf("v1: %d\n", (*inst & VEX2_v1)!= 0);
            printf("v0: %d\n", (*inst & VEX2_v0)!= 0);
            printf("L: %d\n", (*inst & VEX2_L)!= 0);
            printf("p1: %d\n", (*inst & VEX2_p1)!= 0);
            printf("p0: %d\n", (*inst & VEX2_p0)!= 0);

            char p = *inst & 0b11;
            printf("prefix: %s\n", vex_prefix[p]); 
            break;
        default:
            printf("Unknown instruction prefix: %02x\n", *inst);
            exit(1);
    }
}


void sigill_handler(int sig, siginfo_t *info, void *ucontext) {
    ucontext_t *uc = (ucontext_t *)ucontext;
    printf("Invalid instruction at %p\n", info->si_addr);
    printf("RIP: %llx\n", uc->uc_mcontext->__ss.__rip);
    printf("RSP: %llx\n", uc->uc_mcontext->__ss.__rsp);

    decode_instruction((unsigned char*)info->si_addr);
    exit(1);
}

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
    printf("SIGILL handler installed\n");
}

__attribute__((constructor))
void loadMsg(void)
{
    hello();
    init_sigill_handler();
}
