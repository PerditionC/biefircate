#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"

void stage2(const void *rsdp)
{
	acpi_init(rsdp);
	cputws(u"umm...");
	for (;;);
}
