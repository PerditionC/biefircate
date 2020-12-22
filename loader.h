#ifndef H_LOADER
#define H_LOADER

#include <inttypes.h>

/* acpi.h */
extern void process_acpi_v2_tables(void *);

/* fb-con.h */
extern void init_fb_con(void);

/* rm86.h */
/*
 * Interface for switching between 16-bit real mode & 64-bit long mode. 
 * Modelled after Linux's vm86(...) interface as well as Open Watcom's
 * _DPMISimulateRealModeInterrupt(...).
 */
typedef struct __attribute__((packed)) {
	uint32_t edi, esi, ebp, reserved1, ebx, edx, ecx, eax;
	uint16_t es, ds, fs, gs;
	uint32_t eflags, esp;
	uint16_t ss, ip, cs;
} rm86_regs_t;

extern void rm86_set_trampolines_seg(uint16_t seg);
extern rm86_regs_t *rm86_regs(void);
extern void rm86(void);

#endif
