/*
 *  include/abi/socket.h
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

/* Linux spells this differently. */
#define SO_ACCEPTCONN	SO_ACCEPTCON

/* These aren't (currently) defined by Linux. Watch out for warnings
 * about redefinitions...
 */
#define SO_USELOOPBACK	0xff02
#define SO_ORDREL	0xff03
#define SO_IMASOCKET	0xff04
#define SO_PROTOTYPE	0xff09
