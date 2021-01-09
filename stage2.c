/*
 * Copyright (c) 2020--2021 TK Chia
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <stdbool.h>
#include "truckload.h"

#define X64_PG_P	(1ULL << 0)
#define X64_PG_RW	(1ULL << 1)
#define X64_PG_US	(1ULL << 2)
#define X64_PG_PWT	(1ULL << 3)
#define X64_PG_PCD	(1ULL << 4)
#define X64_PG_PS	(1ULL << 7)
#define X64_PG_G	(1ULL << 8)

static INIT_TEXT void fill_pt(uint64_t *pt, uint64_t start, uint64_t end,
    bool pg_writeback_p, bool pg_writethru_p)
{
	uint64_t caching = 0;
	if (pg_writethru_p)
		caching = X64_PG_PWT;
	else if (!pg_writeback_p)
		caching = X64_PG_PCD;
	while (start < end) {
		uint64_t pte = start | X64_PG_P | X64_PG_RW | caching;
		pt[start >> 12 & 0x1ff] = pte;
		start += PAGE_SIZE;
	}
}

static INIT_TEXT void fill_pd(uint64_t *pd, uint64_t start, uint64_t end,
    bool pg_writeback_p, bool pg_writethru_p, size_t *pt_pgs)
{
	bool pt_writeback_p, pt_writethru_p;
	while (start < end) {
		uint64_t mid = (start >> 21 << 21) + (1ULL << 21), pde, *pt;
		if (mid > end)
			mid = end;
		if (mid == start + (1ULL << 21)) {
			/* 2 MiB "large page". */
			pde = start | X64_PG_P | X64_PG_RW | X64_PG_PS;
			if (pg_writethru_p)
				pde |= X64_PG_PWT;
			else if (!pg_writeback_p)
				pde |= X64_PG_PCD;
			pd[start >> 21 & 0x1ff] = pde;
			if (!start)
				cprintf("  zero ent.: %#" PRIx64,
				    pde);
		} else {
			pde = pd[start >> 21 & 0x1ff];
			if (!pde) {
				pt =
				  (uint64_t *)mem_map_reserve_page_anywhere();
				++*pt_pgs;
				mem_map_get_cacheability((uint64_t)pt, NULL,
				    &pt_writeback_p, &pt_writethru_p);
				pde = (uint64_t)pt | X64_PG_P | X64_PG_RW;
				if (pt_writethru_p)
					pde |= X64_PG_PWT;
				else if (!pt_writeback_p)
					pde |= X64_PG_PCD;
				pd[start >> 30 & 0x1ff] = pde;
			} else
				pt = (uint64_t *)(pde & 0x000ffffffffff000ULL);
			if (!start)
				cprintf("  zero PT: @%p", pt);
			fill_pt(pt, start, mid,
			    pg_writeback_p, pg_writethru_p);
		}
		start = mid;
	}
}

static INIT_TEXT void fill_pdp(uint64_t *pdp, uint64_t start, uint64_t end,
    bool pg_writeback_p, bool pg_writethru_p, size_t *pd_pgs, size_t *pt_pgs)
{
	bool pd_writeback_p, pd_writethru_p;
	while (start < end) {
		uint64_t mid = (start >> 30 << 30) + (1ULL << 30), pdpe, *pd;
		if (mid > end)
			mid = end;
		pdpe = pdp[start >> 30 & 0x1ff];
		if (!pdpe) {
			pd = (uint64_t *)mem_map_reserve_page_anywhere();
			++*pd_pgs;
			mem_map_get_cacheability((uint64_t)pd, NULL,
			    &pd_writeback_p, &pd_writethru_p);
			pdpe = (uint64_t)pd | X64_PG_P | X64_PG_RW;
			if (pd_writethru_p)
				pdpe |= X64_PG_PWT;
			else if (!pd_writeback_p)
				pdpe |= X64_PG_PCD;
			pdp[start >> 30 & 0x1ff] = pdpe;
		} else
			pd = (uint64_t *)(pdpe & 0x000ffffffffff000ULL);
		if (!start)
			cprintf("  zero PD: @%p", pd);
		fill_pd(pd, start, mid, pg_writeback_p, pg_writethru_p,
		    pt_pgs);
		start = mid;
	}
}

static INIT_TEXT void fill_pml4(uint64_t *pml4, uint64_t start, uint64_t end,
    bool pg_writeback_p, bool pg_writethru_p,
    size_t *pdp_pgs, size_t *pd_pgs, size_t *pt_pgs)
{
	bool pdp_writeback_p, pdp_writethru_p;
	while (start < end) {
		uint64_t mid = (start >> 39 << 39) + (1ULL << 39), pml4e, *pdp;
		if (mid > end)
			mid = end;
		pml4e = pml4[start >> 39 & 0x1ff];
		if (!pml4e) {
			pdp = (uint64_t *)mem_map_reserve_page_anywhere();
			++*pdp_pgs;
			mem_map_get_cacheability((uint64_t)pdp, NULL,
			    &pdp_writeback_p, &pdp_writethru_p);
			pml4e = (uint64_t)pdp | X64_PG_P | X64_PG_RW;
			if (pdp_writethru_p)
				pml4e |= X64_PG_PWT;
			else if (!pdp_writeback_p)
				pml4e |= X64_PG_PCD;
			pml4[start >> 39 & 0x1ff] = pml4e;
		} else
			pdp = (uint64_t *)(pml4e & 0x000ffffffffff000ULL);
		if (!start)
			cprintf("  zero PDP: @%p", pdp);
		fill_pdp(pdp, start, mid, pg_writeback_p, pg_writethru_p,
		    pd_pgs, pt_pgs);
		start = mid;
	}
}

static INIT_TEXT uint64_t *set_up_page_tables(void)
{
	uint64_t *pml4;
	uint64_t start, end, all_end;
	bool writeback_p, writethru_p;
	size_t pdp_pgs = 0, pd_pgs = 0, pt_pgs = 0;
	cputs("setting up new page tables for long mode\n");
	all_end = mem_map_all_end();
	all_end = (all_end + LARGE_PAGE_SIZE - 1) & -LARGE_PAGE_SIZE;
	pml4 = (uint64_t *)mem_map_reserve_page(0x100000000ULL);
	cprintf("  PML4: @%p", pml4);
	start = 0;
	while (start < all_end) {
		mem_map_get_cacheability(start, &end,
		    &writeback_p, &writethru_p);
		if (end > all_end)
			end = all_end;
		fill_pml4(pml4, start, end, writeback_p, writethru_p,
		    &pdp_pgs, &pd_pgs, &pt_pgs);
		start = end;
	}
	cprintf("\n"
		"  total pages for PML4s: 1  PDPs: %#zu  PDs: %#zu  "
							"PTs: %#zu\n",
	    pdp_pgs, pd_pgs, pt_pgs);
	return pml4;
}

INIT_TEXT void stage2(void)
{
	void *reserved_base_mem;
	uint64_t *pml4 = set_up_page_tables();
	mem_map_free_bs();
	reserved_base_mem = mem_map_reserve_page(0xf0000ULL);
	cprintf("installing LM \u2194 RM trampolines @%p; starting new "
		"LM env.\n", reserved_base_mem);
	lm86_rm86_init(reserved_base_mem, pml4);
}
