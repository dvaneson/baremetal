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
 * kernel.c:
 * Mark P Jones + Donovan Ellison, Portland State University
 *-----------------------------------------------------------------------*/
#include "context.h"
#include "memory.h"
#include "mimguser.h"
#include "paging.h"
#include "simpleio.h"

// Needs these defines to prevent IDE error messages
#ifndef KERNEL_SPACE
#define KERNEL_SPACE 0
#endif

#ifndef KERNEL_LOAD
#define KERNEL_LOAD 0
#endif

/*-------------------------------------------------------------------------
 * Basic code for halting the processor and reporting a fatal error:
 */
extern void halt();

void fatal(char *msg) {
    printf("FATAL ERROR: %s\n", msg);
    halt();
}

/*-------------------------------------------------------------------------
 * Memory management: simple functionality for allocating pages of
 * memory for use in constructing page tables, etc.
 */
unsigned physStart;  // Set during initialization to start of memory pool
unsigned physEnd;    // Set during initialization to end of memory pool

unsigned *allocPage() {
    // DONE (Step 4): Allocate a page of data, starting at
    // the physical address that is specified by physStart,
    // but returning a pointer to the region in virtual
    // memory. The implementation should:
    //   + Trigger a fatal error if there is not enough
    //     space left in [physStart .. physEnd)
    //   + Write zero in to every byte (or word, for a
    //     more efficient implementation) of the page
    //     before returning a pointer.
    //   + Update physStart so that it points to the
    //     next available page in memory after the
    //     allocation is complete ...
    // Be very careful to distinguish between places in your
    // code where you are referring to physical addresses
    // (often using unsigned values, like the value of the
    // variable physStart) and places where you are referring
    // to virtual addresses (often using a variable with a
    // pointer type, for example when you are writing zero
    // bytes in to memory).  It is very easy to consfuse
    // these different kinds of address, but at least it is
    // also easy to convert between them using the fromPhys
    // and toPhys macros ...

    unsigned *page = 0;
    // The number of words in a page, 2^12 / 2^2 = 2^10 = 1024
    int page_words = (1 << PAGESIZE) >> 2;

    // Trigger a fatal error if there is not enough space left
    if (physEnd < pageEnd(physStart)) {
        fatal("Could not allocate a page of data, not enough memory.");
    }

    // Write 0 to every word of the page
    page = fromPhys(unsigned *, physStart);
    for (int i = 0; i < page_words; ++i) {
        page[i] = 0;
    }

    // Update physStart to next available page
    physStart = pageNext(physStart);

    return page;
}

/*-------------------------------------------------------------------------
 * Context data structures: a place holder for when we get back to
 * context switching ...
 */
struct Context user;

/*-------------------------------------------------------------------------
 * The main "kernel" code:
 */
void kernel() {
    struct BootData *bd = (struct BootData *)0x1000;
    unsigned *hdrs = bd->headers;
    unsigned *mmap = bd->mmap;
    unsigned i;
    unsigned start, end;

    setAttr(0x2e);
    cls();
    setAttr(7);
    setWindow(1, 23, 1, 45);  // kernel on left hand side
    cls();
    printf("Paging kernel has booted!\n");

    printf("Headers:\n");
    for (i = 0; i < hdrs[0]; i++) {
        printf(" header[%d]: [%x-%x], entry %x\n", i, hdrs[3 * i + 1],
               hdrs[3 * i + 2], hdrs[3 * i + 3]);
    }

    printf("Memory map:\n");
    for (i = 0; i < mmap[0]; i++) {
        printf(" mmap[%d]: [%x-%x]\n", i, mmap[2 * i + 1], mmap[2 * i + 2]);
    }

    printf("Strings:\n");
    printf(" cmdline: %s [%x]\n", bd->cmdline, bd->cmdline);
    printf(" imgline: %s [%x]\n", bd->imgline, bd->imgline);

    extern struct Pdir initdir[];
    printf("initial page directory is at 0x%x\n", initdir);
    showPdir(initdir);

    printf("kernel code is at 0x%x\n", kernel);

    // DONE (Step 3): Scan the memory map to find the biggest region of
    // available pages subject to the constraints:
    //  - The start address (which should be stored in physStart)
    //    must have a zero page offset and must be >= KERNEL_LOAD,
    //    the address at which the kernel is loaded.
    //  - The end address (which should be stored in physEnd) must
    //    have an offset of 0xfff (i.e., corresponding to the last
    //    byte in a page); it must be greater than physStart; and
    //    it must be less than PHYSMAP.  (It cannot be equal to
    //    PHYSMAP because PHYSMAP has zero offset.)
    // By all means include some printf() calls in your code so
    // that you can trace its execution and be comfortable that
    // it is working correctly.

    physStart = 0;
    physEnd = 0;
    printf("\nConsidering:\n");
    for (i = 0; i < mmap[0]; i++) {
        start = firstPageAfter(mmap[2 * i + 1]);
        end = endPageBefore(mmap[2 * i + 2]);
        printf("  [%08x-%08x], full pages [%08x-%08x]\n", mmap[2 * i + 1],
               mmap[2 * i + 2], start, end);

        if (start >= KERNEL_LOAD && end < PHYSMAP && start < end) {
            if (physEnd - physStart < end - start) {
                physStart = start;
                physEnd = end;
            }
        }
    }

    // DONE (Step 3): Report a fatal error if there is no suitable
    // region of memory.

    if (physEnd <= physStart) {
        fatal("Could not find a valid region in memory map for pages.");
    }

    printf("\nChosen region [%08x-%08x]\n", physStart, physEnd);

    // DONE (Step 3): Scan the list of headers for loaded regions of
    // memory to look for conflicts with the [physStart..physEnd)
    // region.  If you find a conflict, increase physStart to point
    // to the start of the first page after the conflicting region.

    printf("\nHeader conflicts:\n");
    for (i = 0; i < hdrs[0]; i++) {
        start = hdrs[3 * i + 1];
        end = hdrs[3 * i + 2];
        if (end < physEnd && end > physStart)
            physStart = firstPageAfter(end + 1);
        if (start > physStart && start < physEnd)
            physEnd = endPageBefore(start - 1);

        printf("  header[%d] = [%08x-%08x], updated region [%08x-%08x]\n", i,
               start, end, physStart, physEnd);
    }

    // DONE (Step 3): Report a fatal error if this process ends with
    // an empty region of physical memory.

    if (physEnd <= physStart) {
        fatal("After shrinking region for headers, region is invalid");
    }

    // Display the upper and lower bounds of the chosen memory
    // region, as well as the total number of bytes that it
    // contains.

    printf("\nWill allocate from region [%08x-%08x], %d bytes\n", physStart,
           physEnd, 1 + physEnd - physStart);

    // Now we will build a new page directory:
    struct Pdir *newpdir = allocPdir();

    // TODO (Step 8): You might need to do something more to the
    // newly created page directory, but who knows what that might
    // be ... ?   :-)

    showPdir(newpdir);
    for (int i = 0; i < 4; ++i) {
        newpdir = allocPdir();
        showPdir(newpdir);
    }
    halt();

#ifdef IM_FEELING_LUCKY
    setPdir(toPhys(newpdir));
    printf("This message should appear on the screen!\n");
#else
    printf("Switch to the new page directory when you're ready!\n");
#endif

    // TODO: reinstate the following code ... but we'll get to that
    // next time!
    // printf("user code is at 0x%x\n", hdrs[9]);
    // initContext(&user, hdrs[9], 0);
    // printf("user is at %x\n", (unsigned)(&user));
    // switchToUser(&user);

    printf("The kernel will now halt!\n");
    halt();
}

void kputc_imp() { /* A trivial system call */
    putchar(user.regs.eax);
    switchToUser(&user);
}

/*-----------------------------------------------------------------------*/
