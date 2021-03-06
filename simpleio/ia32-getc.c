/* Derived from Karlsruhe code, but blame me not them if my hacks have
   broken it ...
*/
/*********************************************************************
 *                
 * Copyright (C) 2001-2004,  Karlsruhe University
 *                
 * File path:     pistachio.current/user/lib/io/ia32-getc.cc
 * Description:   getc() for x86-based PCs
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 * $Id: ia32-getc.c,v 1.1 2010/03/04 06:19:39 user Exp $
 *                
 ********************************************************************/

/* No SHIFT key support!!! */

#define KBD_STATUS_REG		0x64	
#define KBD_CNTL_REG		0x64	
#define KBD_DATA_REG		0x60	

#define KBD_STAT_OBF 		0x01	/* Keyboard output buffer full */

static inline unsigned char inb(const unsigned short port) {
    unsigned char val;
    __asm__ __volatile__ ("inb  %w1, %0" : "=a"(val) : "dN"(port));
    return val;
}

#define kbd_read_input() inb(KBD_DATA_REG)
#define kbd_read_status() inb(KBD_STATUS_REG)

static unsigned char keyb_layout[128] =
	"\000\0331234567890-+\177\t"			/* 0x00 - 0x0f */
	"qwertyuiop[]\r\000as"				/* 0x10 - 0x1f */
	"dfghjkl;'`\000\\zxcv"				/* 0x20 - 0x2f */
	"bnm,./\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
	"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
	"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
	"\r\000/";					/* 0x60 - 0x6f */


int getc()
{
    static unsigned char last_key = 0;
    char c;
    while(1) {
	unsigned char status = kbd_read_status();
	while (status & KBD_STAT_OBF) {
	    unsigned char scancode;
	    scancode = kbd_read_input();
	    if (scancode & 0x80)
		last_key = 0;
	    else if (last_key != scancode)
	    {
		//printf("kbd: %d, %d, %c\n", scancode, last_key, keyb_layout[scancode]);
		last_key = scancode;
		c = keyb_layout[scancode];
		if (c > 0) return c;
	    }
	}
    }
}

