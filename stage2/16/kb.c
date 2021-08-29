/*
 * Copyright (c) 2021 TK Chia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 *   * The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The logic is loosely derived from Eyal Abraham's new-xt-bios code.  Some
 * bits are based on the IBM PC AT BIOS source listings.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "stage2/stage2.h"

/* 8042 keyboard controller I/O port numbers. */
#define KB_PORT_A	0x0060		/* scan code/control port */
#define KB_PORT_B	0x0061		/* read/write diagnostic register */
#define KB_STA		0x0064		/* 8042 status */

/* 8042 status port bits. */
#define KB_STA_OUT_FULL	0x01		/* output buffer full */
#define KB_STA_IN_FULL	0x02		/* input buffer full */
#define KB_STA_SYS	0x04		/* system flag */
#define KB_STA_DATA	0x08		/* command/data */
#define KB_STA_INH	0x10		/* keyboard inhibited by lock */
#define KB_STA_TX_TMOUT	0x20		/* transmit timeout */
#define KB_STA_RX_TMOUT	0x40		/* receive timeout */
#define KB_STA_PAR_ERR	0x80		/* parity error */

/* 8042 commands to send to port A. */
#define KB_A_LED	0xed		/* set LEDs */

/* 8042 commands to send to the status port. */
#define KB_STA_DIS	0xad		/* disable keyboard */
#define KB_STA_ENA	0xae		/* enable keyboard */

/* 8042 responses. */
#define KB_R_OVERRUN	0xff		/* overrun */
#define KB_R_RESEND	0xfe		/* resend last kbd. cmd. */
#define KB_R_ACK	0xfa		/* acknowledge */
#define KB_R_BREAK	0xf0		/* keyboard break (?) */
#define KB_R_OK		0xaa		/* response from self-diagnostic */

/* 8042 keystroke scan codes & flags. */
#define KB_K_CTL	0x1d		/* Ctrl */
#define KB_K_LSHFT	0x2a		/* left Shift */
#define KB_K_RSHFT	0x36		/* right Shift */
#define KB_K_ALT	0x38		/* Alt */
#define KB_K_CAPLK	0x3a		/* Caps Lock */
#define KB_K_NUMLK	0x45		/* Num Lock */
#define KB_K_SCLLK	0x46		/* Scroll Lock */
#define KB_K_BRK	0x54		/* Break */
#define KB_K_KEY_UP	0x80		/* flag for key-up event */
/* Numeric keypad keys. */
#define KB_K_KP7	0x47		/* Home / 7 */
#define KB_K_KP8	0x48		/* Up / 8 */
#define KB_K_KP9	0x49		/* PgUp / 9 */
#define KB_K_KP4	0x4b		/* Left / 4 */
#define KB_K_KP5	0x4c		/* Centre (?) / 5 */
#define KB_K_KP6	0x4d		/* Right / 6 */
#define KB_K_KP1	0x4f		/* End / 1 */
#define KB_K_KP2	0x50		/* Down / 2 */
#define KB_K_KP3	0x51		/* PgDn / 3 */
#define KB_K_INS	0x52		/* Insert */
#define KB_K_DEL	0x53		/* Delete */
/*
 * Letter keys.
 *	Q  W  E  R  T  Y  U  I  O  P
 *	 A  S  D  F  G  H  J  K  L
 *	  Z  X  C  V  B  N  M
 */
#define KB_K_Q		0x10
#define KB_K_P		0x19
#define KB_K_A		0x1e
#define KB_K_L		0x26
#define KB_K_Z		0x2c
#define KB_K_M		0x32

/* bda.kb_stat1 bit fields. */
#define KB_S1_RSHFT	0x01		/* right Shift pressed */
#define KB_S1_LSHFT	0x02		/* left Shift pressed */
#define KB_S1_CTL	0x04		/* Ctrl pressed */
#define KB_S1_ALT	0x08		/* Alt pressed */
#define KB_S1_SCLLK_ACT	0x10		/* Scroll Lock active */
#define KB_S1_NUMLK_ACT	0x20		/* Num Lock active */
#define KB_S1_CAPLK_ACT	0x40		/* Caps Lock active */
#define KB_S1_INS_ACT	0x80		/* Insert active */

/* bda.kb_stat2 bit fields. */
#define KB_S2_LCTL	0x01		/* left Ctrl pressed */
#define KB_S2_LALT	0x02		/* left Alt pressed */
#define KB_S2_SREQ	0x04		/* SysRq pressed */
#define KB_S2_PAUSE	0x08		/* pause state active */
#define KB_S2_SCLLK	0x10		/* Scroll Lock pressed */
#define KB_S2_NUMLK	0x20		/* Num Lock pressed */
#define KB_S2_CAPLK	0x40		/* Caps Lock pressed */
#define KB_S2_INS	0x80		/* Insert pressed */

static void wait_kb_in_buf_clear(void)
{
	/* Assume interrupts are disabled. */
	uint16_t cnt = 0;
	while (--cnt != 0 && (inp_w(KB_STA) & KB_STA_IN_FULL) != 0);
}

/*
 * Send a command to the 8042 status port (to disable or enable the
 * keyboard).  This assumes interrupts are disabled.
 */
static void snd_sta(uint8_t cmd)
{
	wait_kb_in_buf_clear();
	outp(KB_STA, cmd);
}

static void start_upd_leds(void)
{
}

static void argh(void)
{
}

static void handle_code(uint8_t code)
{
	static DATA16 const uint16_t no_shift_map[] = {
		0x0000, 0x011b, 0x0231, 0x0332, 0x0433, 0x0534, 0x0635, 0x0736,
		0x0837, 0x0938, 0x0a39, 0x0b30, 0x0c2d, 0x0d3d, 0x0e08, 0x0f09,
		0x1071, 0x1177, 0x1265, 0x1372, 0x1474, 0x1579, 0x1675, 0x1769,
		0x186f, 0x1970, 0x1a5b, 0x1b5d, 0x1c0d, 0x0000, 0x1e61, 0x1f73,
		0x2064, 0x2166, 0x2267, 0x2368, 0x246a, 0x256b, 0x266c, 0x273b,
		0x2827, 0x2960, 0x0000, 0x2b5c, 0x2c7a, 0x2d78, 0x2e63, 0x2f76,
		0x3062, 0x316e, 0x326d, 0x332c, 0x342e, 0x352f, 0x0000, 0x372a,
		0x0000, 0x3920, 0x0000, 0x3b00, 0x3c00, 0x3d00, 0x3e00, 0x3f00,
		0x4000, 0x4100, 0x4200, 0x4300, 0x4400, 0x0000, 0x0000, 0x4700,
		0x4800, 0x4900, 0x4a2d, 0x4b00, 0x4c00, 0x4d00, 0x4e2b, 0x4f00,
		0x5000, 0x5100, 0x5200, 0x5300
	    };
	static DATA16 const uint16_t shift_map[] = {
		0x0000, 0x011b, 0x0221, 0x0340, 0x0423, 0x0524, 0x0625, 0x075e,
		0x0826, 0x092a, 0x0a28, 0x0b29, 0x0c5f, 0x0d2b, 0x0e08, 0x0f00,
		0x1051, 0x1157, 0x1245, 0x1352, 0x1454, 0x1559, 0x1655, 0x1749,
		0x184f, 0x1950, 0x1a7b, 0x1b7d, 0x1c0d, 0x0000, 0x1e41, 0x1f53,
		0x2044, 0x2146, 0x2247, 0x2348, 0x244a, 0x254b, 0x264c, 0x273a,
		0x2822, 0x297e, 0x0000, 0x2b7c, 0x2c5a, 0x2d58, 0x2e43, 0x2f56,
		0x3042, 0x314e, 0x324d, 0x333c, 0x343e, 0x353f, 0x0000, 0x0000,
		0x0000, 0x3920, 0x0000, 0x5400, 0x5500, 0x5600, 0x5700, 0x5800,
		0x5900, 0x5a00, 0x5b00, 0x5c00, 0x5d00, 0x0000, 0x0000, 0x4737,
		0x4838, 0x4939, 0x4a2d, 0x4b34, 0x4c35, 0x4d36, 0x4e2b, 0x4f31,
		0x5032, 0x5133, 0x5230, 0x532e
	    };
	static DATA16 const uint16_t ctl_map[] = {
		0x0000, 0x011b, 0x0000, 0x0300, 0x0000, 0x0000, 0x0000, 0x071e,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0c1f, 0x0000, 0x0e7f, 0x9400,
		0x1011, 0x1117, 0x1205, 0x1312, 0x1414, 0x1519, 0x1615, 0x1709,
		0x180f, 0x1910, 0x1a1b, 0x1b1d, 0x1c0a, 0x0000, 0x1e01, 0x1f13,
		0x2004, 0x2106, 0x2207, 0x2308, 0x240a, 0x250b, 0x260c, 0x0000,
		0x0000, 0x0000, 0x0000, 0x2b1c, 0x2c1a, 0x2d18, 0x2e03, 0x2f16,
		0x3002, 0x310e, 0x320d, 0x0000, 0x0000, 0x0000, 0x0000, 0x9600,
		0x0000, 0x3920, 0x0000, 0x5e00, 0x5f00, 0x6000, 0x6100, 0x6200,
		0x6300, 0x6400, 0x6500, 0x6600, 0x6700, 0x0000, 0x0000, 0x7700,
		0x8d00, 0x8400, 0x8e00, 0x7300, 0x8f00, 0x7400, 0x0000, 0x7500,
		0x9100, 0x7600, 0x9200, 0x9300
	    };
	/*
	 * For now, all key codes produced with the Alt key have no
	 * corresponding ASCII character; we only need to store the "scan
	 * code" portion of each such key code.
	 */
	static DATA16 const uint8_t alt_map[] = {
		0x00, 0x01, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d,
		0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x0e, 0xa5,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0xa6, 0x00, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x00, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x00, 0x37,
		0x00, 0x39, 0x00, 0x68, 0x69, 0x6a, 0x6b, 0x6c,
		0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x00, 0x00, 0x97,
		0x98, 0x99, 0x4a, 0x9b, 0x9c, 0x9d, 0x4e, 0x9f,
		0xa0, 0xa1, 0xa2, 0xa3
	    };
	enum {
		NO_SHIFT_MAX = sizeof(no_shift_map) / sizeof(no_shift_map[0]),
		SHIFT_MAX = sizeof(shift_map) / sizeof(shift_map[0]),
		CTL_MAX = sizeof(ctl_map) / sizeof(ctl_map[0]),
		ALT_MAX = sizeof(alt_map) / sizeof(alt_map[0])
	};
	bool shifted = false;
	uint16_t key = 0, old_tail, new_tail;
	switch (code) {
	    case KB_R_OVERRUN:
		argh();				break;
	    case KB_K_LSHFT:
		bda.kb_stat1 |= KB_S1_LSHFT;	break;
	    case KB_K_LSHFT | KB_K_KEY_UP:
		bda.kb_stat1 &= ~KB_S1_LSHFT;	break;
	    case KB_K_RSHFT:
		bda.kb_stat1 |= KB_S1_RSHFT;	break;
	    case KB_K_RSHFT | KB_K_KEY_UP:
		bda.kb_stat1 &= ~KB_S1_RSHFT;	break;
	    case KB_K_CTL:
		bda.kb_stat1 |= KB_S1_CTL;	break;
	    case KB_K_CTL | KB_K_KEY_UP:
		bda.kb_stat1 &= ~KB_S1_CTL;	break;
	    case KB_K_ALT:
		bda.kb_stat1 |= KB_S1_ALT;	break;
	    case KB_K_ALT | KB_K_KEY_UP:
		bda.kb_stat1 &= ~KB_S1_ALT;	break;
	    case KB_K_CAPLK:
		bda.kb_stat1 ^= KB_S1_CAPLK_ACT;
		bda.kb_stat2 |= KB_S2_CAPLK;
		start_upd_leds();
		break;
	    case KB_K_CAPLK | KB_K_KEY_UP:
		bda.kb_stat2 &= ~KB_S2_CAPLK;	break;
	    case KB_K_NUMLK:
		bda.kb_stat1 ^= KB_S1_NUMLK_ACT;
		bda.kb_stat2 |= KB_S2_NUMLK;
		start_upd_leds();
		break;
	    case KB_K_NUMLK | KB_K_KEY_UP:
		bda.kb_stat2 &= ~KB_S2_NUMLK;	break;
	    case KB_K_SCLLK:
		bda.kb_stat1 ^= KB_S1_SCLLK_ACT;
		bda.kb_stat2 |= KB_S2_SCLLK;
		start_upd_leds();
		break;
	    case KB_K_SCLLK | KB_K_KEY_UP:
		bda.kb_stat2 &= ~KB_S2_SCLLK;	break;
	    case KB_K_KP7 ... KB_K_DEL:
		if ((bda.kb_stat1 & KB_S1_NUMLK_ACT) != 0)
			shifted = true;
		goto no_caps;
	    case KB_K_Q ... KB_K_P:
	    case KB_K_A ... KB_K_L:
	    case KB_K_Z ... KB_K_M:
		if ((bda.kb_stat1 & KB_S1_CAPLK_ACT) != 0)
			shifted = true;
		/* fall through */
	    no_caps:
	    default:
		if ((bda.kb_stat1 & KB_S1_ALT) != 0) {
			if (code < ALT_MAX)
				key = (uint16_t)alt_map[code] << 8;
		} else if ((bda.kb_stat1 & KB_S1_CTL) != 0) {
			if (code < CTL_MAX)
				key = ctl_map[code];
		} else {
			if ((bda.kb_stat1 & (KB_S1_RSHFT | KB_S1_LSHFT)) != 0)
				shifted = !shifted;
			if (shifted) {
				if (code < SHIFT_MAX)
					key = shift_map[code];
			} else if (code < NO_SHIFT_MAX)
				key = no_shift_map[code];
		}
	}
	if (!key)
		return;
	old_tail = bda.kb_buf_tail;
	new_tail = old_tail + 2;
	if ((uint16_t)(bda.kb_buf_end - new_tail) < 2)
		new_tail = bda.kb_buf_start;
	if (new_tail == bda.kb_buf_head) {
		/*
		 * Adding another keystroke will make the tail touch the
		 * head, i.e. keyboard buffer in main memory is full...
		 */
		argh();
		return;
	}
	poke(BDA_SEG, old_tail, key);
	bda.kb_buf_tail = new_tail;
}

void irq1_impl(void)
{
	snd_sta(KB_STA_DIS);
	wait_kb_in_buf_clear();
	handle_code(inp(KB_PORT_A));
	outp(PIC1_CMD, OCW2_EOI);
	snd_sta(KB_STA_ENA);
}
