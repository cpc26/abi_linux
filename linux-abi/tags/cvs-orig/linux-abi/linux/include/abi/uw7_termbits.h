#ifndef __ABI_UW7_TERMBITS_H__
#define __ABI_UW7_TERMBITS_H__

#ifdef __KERNEL__

#define UW7_TIOC		('T'<<8)
#define UW7_TCGETA		(UW7_TIOC|1)
#define UW7_TCSETA		(UW7_TIOC|2)
#define UW7_TCSETAW		(UW7_TIOC|3)
#define UW7_TCSETAF		(UW7_TIOC|4)
#define UW7_TCSBRK		(UW7_TIOC|5)
#define UW7_TCXONC		(UW7_TIOC|6)
#define UW7_TCFLSH		(UW7_TIOC|7)
#define UW7_TCDSET		(UW7_TIOC|32)
#define UW7_RTS_TOG		(UW7_TIOC|33)
#define UW7_TIOCGWINSZ		(UW7_TIOC|104)
#define UW7_TIOCSWINSZ		(UW7_TIOC|103)
#define UW7_TCGETS		(UW7_TIOC|13)
#define UW7_TCSETS		(UW7_TIOC|14)
#define	UW7_TCSANOW		UW7_TCSETS
#define UW7_TCSETSW		(UW7_TIOC|15)
#define UW7_TCSADRAIN		UW7_TCSETSW
#define UW7_TCSETSF		(UW7_TIOC|16)
#define UW7_TCSAFLUSH		UW7_TCSETSF

/*
 * VEOF/VEOL and VMIN/VTIME are overloaded.  
 * VEOF/VEOL are used in canonical mode (ICANON),
 * otherwise VMIN/VTIME are used.
 */
#define UW7_VINTR		0
#define UW7_VQUIT		1
#define UW7_VERASE		2
#define UW7_VKILL		3
#define UW7_VEOF		4
#define UW7_VEOL		5
#define UW7_VEOL2		6
#define UW7_VMIN		4
#define UW7_VTIME		5
#define UW7_VSWTCH		7
#define UW7_VSTART		8
#define UW7_VSTOP		9
#define UW7_VSUSP		10
#define UW7_VDSUSP		11
#define UW7_VREPRINT		12
#define UW7_VDISCARD		13
#define UW7_VWERASE		14
#define UW7_VLNEXT		15

/*
 * Input modes (c_iflag), same as Linux bits, except DOSMODE (obsolete).
 */
#define UW7_IFLAG_MSK	0017777
#define UW7_IGNBRK	0000001
#define UW7_BRKINT	0000002
#define UW7_IGNPAR	0000004
#define UW7_PARMRK	0000010
#define UW7_INPCK	0000020
#define UW7_ISTRIP	0000040
#define UW7_INLCR	0000100
#define UW7_IGNCR	0000200
#define UW7_ICRNL	0000400
#define UW7_IUCLC	0001000
#define UW7_IXON	0002000
#define UW7_IXANY	0004000
#define UW7_IXOFF	0010000
#define UW7_IMAXBEL	0020000
#define UW7_DOSMODE	0100000

/*
 * Output modes (c_oflag), exactly the same as Linux bits.
 */
#define UW7_OFLAG_MSK	0177777
#define UW7_OPOST	0000001
#define UW7_OLCUC	0000002
#define UW7_ONLCR	0000004
#define UW7_OCRNL	0000010
#define UW7_ONOCR	0000020
#define UW7_ONLRET	0000040
#define UW7_OFILL	0000100
#define UW7_OFDEL	0000200
#define UW7_NLDLY	0000400
#define   UW7_NL0	0000000
#define   UW7_NL1	0000400
#define UW7_CRDLY	0003000
#define   UW7_CR0	0000000
#define   UW7_CR1	0001000
#define   UW7_CR2	0002000
#define   UW7_CR3	0003000
#define UW7_TABDLY	0014000
#define   UW7_TAB0	0000000
#define   UW7_TAB1	0004000
#define   UW7_TAB2	0010000
#define   UW7_TAB3	0014000
#define   UW7_XTABS	UW7_TAB3
#define UW7_BSDLY	0020000
#define   UW7_BS0	0000000
#define   UW7_BS1	0020000
#define UW7_VTDLY	0040000
#define   UW7_VT0	0000000
#define   UW7_VT1	0040000
#define UW7_FFDLY	0100000
#define   UW7_FF0	0000000
#define   UW7_FF1	0100000

/*
 * Control modes (c_cflag).
 */
#define UW7_CFLAG_MSK	0177777
#define UW7_CBAUD	0000017
#define UW7_CSIZE	0000060
#define   UW7_CS5	0000000
#define   UW7_CS6	0000020
#define   UW7_CS7	0000040
#define   UW7_CS8	0000060
#define UW7_CSTOPB	0000100
#define UW7_CREAD	0000200
#define UW7_PARENB	0000400
#define UW7_PARODD	0001000
#define UW7_HUPCL	0002000
#define UW7_CLOCAL	0004000
#define UW7_XCLUDE	0100000
#define UW7_CIBAUD	000003600000
#define UW7_IBSHIFT	16      
#define UW7_PAREXT	000004000000

/*
 * Local modes (c_lflag), same as Linux except 
 * UW7_FLUSHO is different and UW7_DEFECHO is obsolete (set to 0).
 */
#define UW7_LFLAG_MSK	0001777
#define UW7_ISIG	0000001
#define UW7_ICANON	0000002
#define UW7_XCASE	0000004
#define UW7_ECHO	0000010
#define UW7_ECHOE	0000020
#define UW7_ECHOK	0000040
#define UW7_ECHONL	0000100
#define UW7_NOFLSH	0000200
#define UW7_TOSTOP	0000400
#define UW7_ECHOCTL	0001000
#define UW7_ECHOPRT	0002000
#define UW7_ECHOKE	0004000
#define UW7_DEFECHO	0010000
#define UW7_FLUSHO	0020000
#define UW7_PENDIN	0040000
#define UW7_IEXTEN	0100000

#endif /* __KERNEL__ */
#endif /* __ABI_UW7_TERMBITS_H__ */
