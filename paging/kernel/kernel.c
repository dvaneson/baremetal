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
#include "hardware.h"
#include "memory.h"
#include "mimguser.h"
#include "paging.h"
#include "simpleio.h"

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
unsigned physStart; // Set during initialization to start of memory pool
unsigned physEnd;   // Set during initialization to end of memory pool

unsigned *allocPage() {
    // Trigger a fatal error if there is not enough space left
    if (physEnd < pageEnd(physStart)) {
        fatal("Could not allocate a page of data, not enough memory.");
    }

    // Write 0 to every word of the page
    unsigned *page = fromPhys(unsigned *, physStart);
    for (unsigned i = 0; i < PAGEWORDS; ++i) {
        page[i] = 0;
    }

    // Update physStart to next available page
    physStart = pageNext(physStart);

    return page;
}

unsigned copyRegion(unsigned lo, unsigned hi) {
    // Check that lo < hi
    if (lo >= hi) {
        fatal("In copyRegion: 'lo' must be less than 'hi'");
    }

    // Ensure that lo and hi correspond to suitable page
    // boundaries.
    if (lo != pageStart(lo) || hi != pageEnd(hi)) {
        fatal("In copyRegion: 'lo' or 'hi' invalid page boundaries");
    }

    // Figure out if there is enough memory left in the
    // pool between physStart and physEnd to make a copy
    // of the data between lo and hi.
    if ((hi - lo) > (physEnd - physStart)) {
        fatal("In copyRegion: Not enough memory to copy region");
    }

    // Keep track of where the new memory region starts
    unsigned start = physStart;

    // Setting up virtual addresses
    unsigned *srcPage = fromPhys(unsigned *, lo);
    unsigned *dstPage = fromPhys(unsigned *, start);

    // Update physStart as necessary.
    printf("\n");
    unsigned numPages = ((hi + 1) - lo) >> PAGESIZE;
    for (int i = 0; i < numPages; ++i) {
        printf("Allocating page [%x-%x]\n", physStart, pageEnd(physStart));
        physStart = pageNext(physStart);
    }

    // Copies long by long for each page allocated
    // TODO Any other tests to make sure this works properly?
    printf("\nCopying from [%x-%x] to [%x-%x]\n", lo, hi, start,
           start + (PAGEBYTES * numPages) - 1);
    for (unsigned i = 0; i < PAGEWORDS * numPages; ++i) {
        dstPage[i] = srcPage[i];
    }
    printf("\n");

    // Return the physical address of the start of the
    // region where the new copy was placed.
    return start;
}

/*-------------------------------------------------------------------------
 * Context data structures: a place holder for when we get back to
 * context switching ...
 */
struct Process {
    struct Context ctxt;
    struct Pdir *pdir;
};
struct Process proc;

/*-------------------------------------------------------------------------
 * The main "kernel" code:
 */
void kernel() {
    struct BootData *bd = fromPhys(struct BootData *, 0x1000);
    unsigned *hdrs = fromPhys(unsigned *, (unsigned)bd->headers);
    unsigned *mmap = fromPhys(unsigned *, (unsigned)bd->mmap);
    unsigned i;
    unsigned start, end;

    setAttr(0x2e);
    cls();
    setAttr(7);
    setWindow(1, 23, 1, 45); // kernel on left hand side
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

    physStart = 0;
    physEnd = 0;
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

    if (physEnd <= physStart) {
        fatal("Could not find a valid region in memory map for pages.");
    }
    for (i = 0; i < hdrs[0]; i++) {
        start = hdrs[3 * i + 1];
        end = hdrs[3 * i + 2];
        if (end < physEnd && end > physStart)
            physStart = firstPageAfter(end + 1);
        if (start > physStart && start < physEnd)
            physEnd = endPageBefore(start - 1);
    }

    if (physEnd <= physStart) {
        fatal("After shrinking region for headers, region is invalid");
    }

    // Now we will build a new page directory:
    proc.pdir = allocPdir();

    // Updating the Page Directory means losing the lower address mappings, so
    // they must be re-mapped
    // 0xb8000 is video ram
    // 0x1000 is where to find the boot data
    mapPage(proc.pdir, 0xb8000, 0xb8000);
    mapPage(proc.pdir, 0x1000, 0x1000);

    start = pageStart(hdrs[7]);
    end = pageEnd(hdrs[8]);
    unsigned startCopy = copyRegion(start, end);
    unsigned curr = startCopy;
    while (start < end) {
        mapPage(proc.pdir, start, start);
        mapPage(proc.pdir, curr, curr);

        start = pageNext(start);
        curr = pageNext(curr);
    }
    showPdir(proc.pdir);
    setPdir(toPhys(proc.pdir));

    printf("\nSrc user code is at 0x%x\n", hdrs[9]);
    initContext(&proc.ctxt, hdrs[9], 0);
    printf("Src user is at %x\n", (unsigned)(&proc.ctxt));

    unsigned userStart = (hdrs[9] - hdrs[7]) + startCopy;
    printf("\nDst user code is at 0x%x\n", userStart);
    initContext(&proc.ctxt, userStart, 0);
    printf("Dst user is at %x\n", (unsigned)(&proc.ctxt));
    printf("\n");

    startTimer();
    switchToUser(&proc.ctxt);

    printf("The kernel will now halt!\n");
    halt();
}

/*-------------------------------------------------------------------------
 * System calls
 */
// A Trivial system call
void kputc_imp() {
    putchar(proc.ctxt.regs.eax);
    switchToUser(&proc.ctxt);
}

static void tick() {
    static unsigned ticks = 0;
    ticks++;
    if ((ticks & 15) == 0) {
        printf(".");
        // current = (current == user) ? (user + 1) : user;
    }
}

void timerInterrupt() {
    maskAckIRQ(TIMERIRQ);
    enableIRQ(TIMERIRQ);
    tick();
    switchToUser(&proc.ctxt);
}

/*-----------------------------------------------------------------------*/
