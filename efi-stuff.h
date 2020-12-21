#ifndef H_EFI_STUFF
#define H_EFI_STUFF

#include <efi.h>

extern void wait_and_exit(EFI_STATUS);
extern void error_with_status(IN CONST CHAR16 *, EFI_STATUS);
extern void error(IN CONST CHAR16 *);

#endif
