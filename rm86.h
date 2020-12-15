/*
 * Interface for switching between 16-bit real mode & 64-bit long mode. 
 * Modelled after Linux's vm86(...) interface as well as Open Watcom's
 * _DPMISimulateRealModeInterrupt(...).
 */

#ifndef H_RM86
#define H_RM86

#include <efi.h>

typedef struct __attribute__((packed)) {
	UINT32 edi, esi, ebp, reserved1, ebx, edx, ecx, eax;
	UINT16 es, ds, fs, gs;
	UINT32 eflags, esp;
	UINT16 ss, ip, cs;
} rm86_regs_t;

extern void rm86_set_trampolines_seg(UINT16 seg);
extern rm86_regs_t *rm86_regs(void);
extern void rm86(void);

#endif
