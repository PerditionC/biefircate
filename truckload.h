#ifndef H_LOADER
#define H_LOADER

#include <inttypes.h>
#include <uchar.h>

/* acpi.c */
extern void acpi_init(const void *);

/* fb-con.c */
extern void fb_con_init(void);
extern void fb_con_instate(void);
extern void fb_con_exit(void);
extern int cwprintf(const char16_t *, ...);
extern void putwch(char16_t);
extern void cputws(const char16_t *);

/* x64.S */
typedef struct __attribute__((packed)) {
	uint32_t edi, esi, ebp, reserved1, ebx, edx, ecx, eax;
	uint16_t es, ds, fs, gs;
	uint32_t eflags, esp;
	uint16_t ss, ip, cs;
} rm86_regs_t;

extern void lm86_rm86_init(uint16_t);
extern rm86_regs_t *rm86_regs(void);
extern void rm86(void);

/* stage1.c */
void stage1(const void **);

/* stage2.c */
void stage2(const void *);

#endif
