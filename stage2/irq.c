/*
 * Copyright (c) 2021 TK Chia
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the developer(s) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "acpi.h"
#include "common.h"
#include "stage2/stage2.h"

static void process_rsdp(acpi_xsdp_t *rsdp)
{
	static const char expect_rsdp_sig[8] = "RSD PTR ";
	if (memcmp(rsdp->signature, expect_rsdp_sig, 8) != 0)
		hlt();
	/* TODO */
}

void irq_init(bparm_t *bparms)
{
	bdat_rsdp_t *bd_rsdp;
	acpi_xsdp_t *rsdp;
	uint32_t rsdp_sz;
	bparm_t *bp = bparms;
	while (bp->type != BP_RSDP)
		bp = bp->next;
	bd_rsdp = &bp->u->rsdp;
	rsdp_sz = bd_rsdp->rsdp_sz;
	rsdp = mem_va_map(bd_rsdp->rsdp_phy_addr, rsdp_sz, 0);
	process_rsdp(rsdp);
	mem_va_unmap(rsdp, rsdp_sz);
}
