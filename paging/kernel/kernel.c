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
    unsigned *page = 0;

    // Trigger a fatal error if there is not enough space left
    if (physEnd < pageEnd(physStart)) {
        fatal("Could not allocate a page of data, not enough memory.");
    }

    // Write 0 to every word of the page
    page = fromPhys(unsigned *, physStart);
    for (int i = 0; i < PAGEWORDS; ++i) {
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
    struct Pdir *newpdir = allocPdir();

    // Updating the Page Directory means losing the lower address mappings, so
    // they must be re-mapped
    // 0xb8000 is video ram and thus gives access to
    // 0x1000 is where to find the boot data
    mapPage(newpdir, 0xb8000, 0xb8000);
    mapPage(newpdir, 0x1000, 0x1000);

    start = pageStart(hdrs[7]);
    end = pageEnd(hdrs[8]);
    while (start < end) {
        mapPage(newpdir, start, start);
        start = pageNext(start);
    }
    showPdir(newpdir);
    setPdir(toPhys(newpdir));

    printf("user code is at 0x%x\n", hdrs[9]);
    initContext(&user, hdrs[9], 0);
    printf("user is at %x\n", (unsigned)(&user));

    startTimer();
    switchToUser(&user);

    printf("The kernel will now halt!\n");
    halt();
}

void kputc_imp() { /* A trivial system call */
    putchar(user.regs.eax);
    switchToUser(&user);
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
    switchToUser(&user);
}

/*-----------------------------------------------------------------------*/
