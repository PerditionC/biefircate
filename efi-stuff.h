#ifndef H_EFI_STUFF
#define H_EFI_STUFF

#include <efi.h>

/* exit.c */
extern void wait_and_exit(EFI_STATUS);
extern void error_with_status(IN CONST CHAR16 *, EFI_STATUS);
extern void error(IN CONST CHAR16 *);

/* mem-map.c */
extern void mem_map_init(UINTN *);
extern void stage1_done(UINTN);

#endif
