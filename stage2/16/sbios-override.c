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

/* Code to override certain SeaBIOS routines with our custom code. */

#include <inttypes.h>

/* Enable IRQs for a short while. */
void check_irqs(void)
{
	__asm volatile("sti; nop ; nop; rep ; nop; cli; cld" : : : "memory");
}

/* Say whether we are on SeaBIOS's "extra 16-bit stack". */
int on_extra_stack(void)
{
	return 0;
}

/*
 * Reset the system.  Here I induce a triple fault to do a true system
 * reset.
 */
void reset(void)
{
	extern struct __attribute__((packed)) {
		uint16_t length;
		uint32_t addr;
	} pmode_IDT_info;
	__asm volatile("lidt %%cs:%0; int3"
		       : : "g" (pmode_IDT_info) : "memory");
	__builtin_unreachable();
}

/*
 * Read an integer setting from a ROM configuration "file".  This just
 * returns the supplied default integer value.
 */
uint64_t romfile_loadint(const char *name, uint64_t defval)
{
	return defval;
}
