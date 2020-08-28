/*
    Copyright 2014-2018 Mark P Jones, Portland State University

    This file is part of CEMLaBS/LLP Demos and Lab Exercises.

    CEMLaBS/LLP Demos and Lab Exercises is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    CEMLaBS/LLP Demos and Lab Exercises is distributed in the hope that
    it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CEMLaBS/LLP Demos and Lab Exercises.  If not, see
    <https://www.gnu.org/licenses/>.
*/
/*-------------------------------------------------------------------------
 * paging.c:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "paging.h"
#include "memory.h"
#include "winio.h"

extern unsigned *allocPage();
extern unsigned copyRegion(unsigned lo, unsigned hi);
extern void fatal(char *msg);

/*-------------------------------------------------------------------------
 * Allocation of paging structures:
 */
struct Pdir *allocPdir() {
    // Allocate a fresh page directory:
    struct Pdir *pdir = (struct Pdir *)allocPage();
    unsigned kern_entries = (PHYSMAP >> SUPERSIZE);
    unsigned kern_addr = (KERNEL_SPACE >> SUPERSIZE);

    // Add superpage mappings to pdir for the first PHYSMAP bytes of physical
    // memory.
    for (unsigned i = 0; i < kern_entries; ++i) {
        pdir->pde[i + kern_addr] = (i * SUPERBYTES) + PERMS_KERNELSPACE;
    }

    return pdir;
}

struct Ptab *allocPtab() {
    return (struct Ptab *)allocPage();
}

struct Pdir *newUserPdir(unsigned lo, unsigned hi) {
    // Check that lo < hi
    if (lo >= hi) {
        fatal("In newUserPdir: 'lo' must be less than 'hi'");
    }

    // Check that lo and hi are suitable page boundaries.
    if (lo != pageStart(lo) || hi != pageEnd(hi)) {
        fatal("In newUserPdir: 'lo' or 'hi' invalid page boundaries");
    }

    struct Pdir *pdir = allocPdir();
    unsigned phys = copyRegion(lo, hi);

    // Add mappings for the region between lo and hi in the virtual address
    // space to physical adresses starting at phys.
    while (lo < hi) {
        mapPage(pdir, lo, phys);
        phys = pageNext(phys);
        lo = pageNext(lo);
    }

    return pdir;
}

/*-------------------------------------------------------------------------
 * Create a mapping in the specified page directory that will map the
 * virtual address (page) specified by virt to the physical address
 * (page) specified by phys. Any nonzero offset in the least
 * significant 12 bits of either virt or phys will be ignored.
 *
 */
void mapPage(struct Pdir *pdir, unsigned virt, unsigned phys) {
    // Mask out the least significant 12 bits of virt and phys.
    virt = alignTo(virt, PAGESIZE);
    phys = alignTo(phys, PAGESIZE);

    unsigned pde_index = (virt >> SUPERSIZE);
    unsigned pte_index = maskTo(virt, SUPERSIZE) >> PAGESIZE;

    if (pde_index >= PAGEWORDS) {
        fatal("PDE index out of bounds");
    }

    // Find the relevant entry in the page directory and check if it's valid
    unsigned pde = pdir->pde[pde_index];
    struct Ptab *ptab = 0;
    if (pde & 1) {
        // Check if the PDE maps to a superpage
        if (pde & PERMS_SUPERPAGE) {
            fatal("PDE maps to a superpage");
        }

        // Clear the lower 12 bits to get the Page Table's physical address
        ptab = fromPhys(struct Ptab *, alignTo(pde, PAGESIZE));

        // Check if the PDE is already in use.
        if (ptab->pte[pte_index] & 1) {
            fatal("PTE already mapped to a physical address");
        }
    } else {
        // Allocate a new page table and update the pdir to point to it.
        ptab = (struct Ptab *)allocPage();
        pdir->pde[pde_index] = toPhys(ptab) + PERMS_USER_RW;
    }

    // The PTE is a 20-bit physical adress with a 12-bit offset
    ptab->pte[pte_index] = phys + PERMS_USER_RW;
}

/*-------------------------------------------------------------------------
 * Print a description of a page directory (for debugging purposes).
 */
void showPdir(struct Pdir *pdir) {
    printf("  Page directory at %x\n", pdir);
    for (unsigned i = 0; i < 1024; i++) {
        if (pdir->pde[i] & 1) {
            if (pdir->pde[i] & 0x80) {
                printf("    %3x: [%x-%x] => [%x-%x], superpage\n", i,
                       (i << SUPERSIZE), ((i + 1) << SUPERSIZE) - 1,
                       alignTo(pdir->pde[i], SUPERSIZE),
                       alignTo(pdir->pde[i], SUPERSIZE) + 0x3fffff);
            } else {
                struct Ptab *ptab =
                    fromPhys(struct Ptab *, alignTo(pdir->pde[i], PAGESIZE));
                unsigned base = (i << SUPERSIZE);
                printf("    %3x: [%x-%x] => page table at %x (physical %x):\n",
                       i, base, base + (1 << SUPERSIZE) - 1, ptab,
                       alignTo(pdir->pde[i], PAGESIZE));
                for (unsigned j = 0; j < 1024; j++) {
                    if (ptab->pte[j] & 1) {
                        printf("        %3x: [%x-%x] => [%x-%x] page\n", j,
                               base + (j << PAGESIZE),
                               base + ((j + 1) << PAGESIZE) - 1,
                               alignTo(ptab->pte[j], PAGESIZE),
                               alignTo(ptab->pte[j], PAGESIZE) + 0xfff);
                    }
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------*/
