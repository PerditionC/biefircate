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
#include "stage2/stage2.h"

static void rimg_init(bparm_t *bparms, bool init_vga)
{
	bparm_t *bp;
	for (bp = bparms; bp; bp = bp->next) {
		uint16_t rimg_seg;
		bdat_pci_dev_t *pd;
		bool do_init;
		if (bp->type != BP_PCID)
			continue;
		pd = &bp->u->pci_dev;
		switch (pd->class_if & 0xffff0000UL) {
		    case 0x03000000:  /* VGA */
		    case 0x03010000:  /* XGA */
			do_init = init_vga;
			break;
		    default:
			do_init = !init_vga;
		}
		if (!do_init)
			continue;
		rimg_seg = pd->rimg_seg;
		if (!rimg_seg)
			continue;
		rm16_call(pd->pci_locn, 0, 0, pd->rimg_rt_seg,
		    MK_FP16(rimg_seg, 0x0003));
	}
}

static void hello(void)
{
	extern void hello16(void);
	rm16_call(0, 0, 0, 0, MK_FP16(rm16_cs, (uint16_t)(uintptr_t)hello16));
}

void stage2_main(bparm_t *bparms, void *rm16_load, size_t rm16_sz)
{
	mem_init(bparms);
	rm16_init();
	rimg_init(bparms, true);
	hello();
	rimg_init(bparms, false);
	hlt();
}
