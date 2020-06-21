
	.equ	ROWS, 		25		# number of rows
	.equ	COLS, 		80		# number of columns

	.equ	CHARBYTES,	2		# total #bytes per char
	.equ	ROWBYTES,	COLS*CHARBYTES	# total #bytes per row
	.equ	SCREENBYTES,	ROWS*ROWBYTES	# total #bytes per screen

	.equ	SPACE,		32		# blank space
	.equ	NEWLINE,	'\n'		# newline character

	.equ	DEFAULT_ATTR, 	0x2e		# PSU Green

	.data

        # Reserve space for a video ram frame buffer with
        # 25 rows; 80 columns; and one code and one attribute
        # byte per character.
	.globl  video
	.align	4
video:	.space	SCREENBYTES

    # Some variables to hold the current row, column, and
	# video attribute:
	.align	4
row:	.long	0		# we will only use the least significant
col:	.long	0		# bytes of these variables
attr:	.long	0

	.text

	# Clear the screen, setting all characters to SPACE
    # using the DEFAULT_ATTR attribute.
	.globl	cls
cls:	pushl	%ebp
	movl	%esp, %ebp

	leal video, %eax
	movl $(SCREENBYTES >> 2), %ecx	# Counter. Moving longs, so count = SCREENBYTES / 4

	# Set %edx such that the first 16-bits and last 16-bits specify a green space
	movb $(SPACE), %dl
	movb $(DEFAULT_ATTR), %dh
	shl  $16, %edx
	movb $(SPACE), %dl
	movb $(DEFAULT_ATTR), %dh

	# Loop over video array, setting 4 bytes at a time
1:	movl %edx, (%eax)
	addl $4, %eax
	decl %ecx
	jnz 1b

	movl	%ebp, %esp
	popl	%ebp
	ret

	# Set the video attribute for characters output using outc.
	.globl	setAttr
setAttr:pushl	%ebp
	movl	%esp, %ebp

	movl	8(%ebp), %eax
	movl	%eax, attr

	movl	%ebp, %esp
	popl	%ebp
	ret

	# Output a single character at the current row and col position
	# on screen, advancing the cursor coordinates and scrolling the
	# screen as appropriate.  Special treatment is provided for
    # NEWLINE characters, moving the "cursor" to the start of the
	# "next line".
	.globl	outc
outc:	pushl	%ebp
	movl	%esp, %ebp
	pushl 	%edi

	leal video, %eax
	movb 8(%ebp), %dl
	cmpb $(NEWLINE), %dl
	je newline

	movl $(COLS), %ecx
	imull	row, %ecx
	addl col, %ecx
	movb attr, %dh
	movw %dx, (%eax, %ecx, 2)
	incl col
	cmpl $(COLS), col
	jl outc_end

newline:
	movl 	$0, col
	incl 	row
	cmpl 	$(ROWS), row
	jl 		outc_end
	decl	row

scroll:
	# Loop through each row except the last, replacing the current row with the next
	leal (ROWBYTES)(%eax), %edx				# Set %edx to the row right after %eax
	movl $(((ROWS-1) * COLS) >> 1), %ecx 	# Counter for number of rows to update, aside from the last one

1:	movl (%edx), %edi
	movl %edi, (%eax)
	addl $4, %eax
	addl $4, %edx
	decl %ecx
	jnz 1b

	# Loop through the last row, updating each col to a green space
	movb $(SPACE), %dl
	movb $(DEFAULT_ATTR), %dh
	shl  $16, %edx
	movb $(SPACE), %dl
	movb $(DEFAULT_ATTR), %dh
	movl $(ROWBYTES >> 2), %ecx		# Counter for number of double words in a row

	
1:  movl %edx, (%eax)
	addl $4, %eax
	decl %ecx
	jnz 1b

outc_end:
	popl 	%edi
	movl	%ebp, %esp
	popl	%ebp
	ret

	# Output an unsigned numeric value as a sequence of 8 hex digits.
	.globl	outhex
outhex:	pushl	%ebp
	movl	%esp, %ebp
	# Fill me in!
	movl	%ebp, %esp
	popl	%ebp
	ret

