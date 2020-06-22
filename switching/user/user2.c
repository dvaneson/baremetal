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
/* A simple program that we will run in user mode.
 */
#include "simpleio.h"

extern void kputc(unsigned);
extern void yield(void);

void kputs(char *s) {
    while (*s) {
        kputc(*s++);
    }
}

void cmain() {
    int i;
    setWindow(13, 11, 47, 32); // user2 process on lower right hand side
    cls();
    puts("in user2 code\n");
    for (i = 0; i < 4; i++) {
        kputs("hello, from user2\n");
        printf("%3d: hello, user2 console\n", i);
        // yield();
    }
    puts("\n\nUser2 code does not return\n");

    unsigned *flagAddr = (unsigned *)0x402710;
    printf("flagAddr = 0x%x\n", flagAddr);
    *flagAddr = 1234;

    for (;;) { /* Don't return! */
    }
    puts("This message won't appear!\n");
}
