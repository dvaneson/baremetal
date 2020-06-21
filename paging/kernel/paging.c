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
#include "simpleio.h" // For printf (debugging output)

extern unsigned *allocPage();
extern void fatal(char *msg);

/*-------------------------------------------------------------------------
 * Allocation of paging structures:
 */
struct Pdir *allocPdir() {
    // Allocate a fresh page directory:
    struct Pdir *pdir = (struct Pdir *)allocPage();
    unsigned kern_entries = (PHYSMAP >> SUPERSIZE);
    unsigned kern_addr = (KERNEL_SPACE >> SUPERSIZE);

    // DONE (Step 5): Add superpage mappings to pdir for the first PHYSMAP
    // bytes of physical memory.  You should use a bitwise or
    // operation to ensure that the PERMS_KERNELSPACE bits are set
    // in every PDE that you create.

    for (unsigned i = 0; i < kern_entries; ++i) {
        pdir->pde[i + kern_addr] = (i * SUPERBYTES) | PERMS_KERNELSPACE;
    }

    return pdir;
}

struct Ptab *allocPtab() {
    return (struct Ptab *)allocPage();
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

    // DONE (Step 7): Find the relevant entry in the page directory

    // DONE (Step 7): report a fatal error if there is already a
    //       superpage mapped at that address (this shouldn't
    //       be possible at this stage, but we're programming
    //       defensively).

    // DONE (Step 7): If there is no page table (i.e., the PDE is
    //       empty), then allocate a new page table and update the
    //       pdir to point to it.   (use PERMS_USER_RW together with
    //       the new page table's *physical* address for this.)

    // DONE (Step 7): If there was an existing page table (i.e.,
    //       the PDE pointed to a page table), then report a fatal
    //       error if the specific entry we want is already in use.

    // DONE (Step 7): Add an entry in the page table structure to
    //       complete the mapping of virt to phys.  (Use PERMS_USER_RW
    //       again, this time combined with the value of the
    //       phys parameter that was supplied as an input.)

    unsigned pde_index = (virt >> SUPERSIZE);
    unsigned pte_index = maskTo(virt, SUPERSIZE) >> PAGESIZE;
    printf("\nVirt before: %x\n", virt);
    printf("PDE index: %x\n", pde_index);
    printf("PTE index: %x\n", pte_index);

    if (pde_index >= PAGEWORDS) {
        fatal("PDE index out of bounds");
    }

    struct Ptab *ptab = 0;
    unsigned pde = pdir->pde[pde_index];
    if (pde & 1) {
        if (pde & PERMS_SUPERPAGE) {
            fatal("PDE maps to a superpage");
        }
        // Clear the lower 12 bits to get the Page Table's physical address
        ptab = fromPhys(struct Ptab *, alignTo(pde, PAGESIZE));
        if (ptab->pte[pte_index] & 1) {
            fatal("PTE already mapped to a physical address");
        }
    } else {
        ptab = (struct Ptab *)allocPage();
        pdir->pde[pde_index] = toPhys(ptab) + PERMS_USER_RW;
    }

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
                        printf("      %03x: [%x-%x] => [%x-%x] page\n", j,
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
