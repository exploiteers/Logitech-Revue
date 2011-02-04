/* mpi.h  -  Multi Precision Integers
 *	Copyright (C) 1994, 1996, 1998, 1999,
 *                    2000, 2001 Free Software Foundation, Inc.
 *
 * This file is part of GNUPG.
 *
 * GNUPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * Note: This code is heavily based on the GNU MP Library.
 *	 Actually it's the same code with only minor changes in the
 *	 way the data is stored; this is to support the abstraction
 *	 of an optional secure memory allocation which may be used
 *	 to avoid revealing of sensitive data due to paging etc.
 *	 The GNU MP Library itself is published under the LGPL;
 *	 however I decided to publish this code under the plain GPL.
 */

#ifndef G10_MPI_H
#define G10_MPI_H

#include "gnupg_mpi_config.h"
#include "gnupg_mpi_types.h"
#include "gnupg_mpi_memory.h" 

/* DSI defines */

#define SHA1_DIGEST_LENGTH   20

/*end of DSI defines */

#define log_debug printk
#define log_bug printk

#define assert(x) do { \
               if (!x) log_bug("failed assertion\n"); \
	       } while(0);

#if BYTES_PER_MPI_LIMB == SIZEOF_UNSIGNED_INT
  typedef unsigned int mpi_limb_t;
  typedef   signed int mpi_limb_signed_t;
#elif BYTES_PER_MPI_LIMB == SIZEOF_UNSIGNED_LONG
  typedef unsigned long int mpi_limb_t;
  typedef   signed long int mpi_limb_signed_t;
#elif BYTES_PER_MPI_LIMB == SIZEOF_UNSIGNED_LONG_LONG
  typedef unsigned long long int mpi_limb_t;
  typedef   signed long long int mpi_limb_signed_t;
#elif BYTES_PER_MPI_LIMB == SIZEOF_UNSIGNED_SHORT
  typedef unsigned short int mpi_limb_t;
  typedef   signed short int mpi_limb_signed_t;
#else
  #error BYTES_PER_MPI_LIMB does not match any C type
#endif
#define BITS_PER_MPI_LIMB    (8*BYTES_PER_MPI_LIMB)

#ifndef EXTERN_UNLESS_MAIN_MODULE
 #if defined (__riscos__) && !defined (INCLUDED_BY_MAIN_MODULE)
  #define EXTERN_UNLESS_MAIN_MODULE extern
 #else
  #define EXTERN_UNLESS_MAIN_MODULE 
 #endif
#endif

#define DBG_MPI     mpi_debug_mode
EXTERN_UNLESS_MAIN_MODULE int mpi_debug_mode;


struct gcry_mpi {
    int alloced;    /* array size (# of allocated limbs) */
    int nlimbs;     /* number of valid limbs */
    int nbits;	    /* the real number of valid bits (info only) */
    int sign;	    /* indicates a negative number */
    unsigned flags; /* bit 0: array must be allocated in secure memory space */
		    /* bit 1: not used */
		    /* bit 2: the limb is a pointer to some m_alloced data */
    mpi_limb_t *d;  /* array with the limbs */
};

typedef struct gcry_mpi *MPI;

#define MPI_NULL NULL

#define mpi_get_nlimbs(a)     ((a)->nlimbs)
#define mpi_is_neg(a)	      ((a)->sign)

/*-- mpiutil.c --*/

MPI mpi_alloc( unsigned nlimbs );
MPI mpi_alloc_secure( unsigned nlimbs );
MPI mpi_alloc_like( MPI a );
void mpi_free( MPI a );
void mpi_resize( MPI a, unsigned nlimbs );
MPI  mpi_copy( MPI a );
#define mpi_is_opaque(a) ((a) && ((a)->flags&4))
MPI mpi_set_opaque( MPI a, void *p, int len );
void *mpi_get_opaque( MPI a, int *len );
#define mpi_is_secure(a) ((a) && ((a)->flags&1))
void mpi_set_secure( MPI a );
void mpi_clear( MPI a );
void mpi_set( MPI w, MPI u);
void mpi_set_ui( MPI w, ulong u);
MPI  mpi_alloc_set_ui( unsigned long u);
void mpi_m_check( MPI a );
void mpi_swap( MPI a, MPI b);

/*-- mpicoder.c --*/
MPI do_encode_md(const byte *sha_buffer, unsigned nbits);
MPI mpi_read_from_buffer(byte *buffer, unsigned *ret_nread, int secure);
int mpi_fromstr(MPI val, const char *str);
u32 mpi_get_keyid( MPI a, u32 *keyid );
byte *mpi_get_buffer( MPI a, unsigned *nbytes, int *sign );
byte *mpi_get_secure_buffer( MPI a, unsigned *nbytes, int *sign );
void  mpi_set_buffer( MPI a, const byte *buffer, unsigned nbytes, int sign );

#define log_mpidump g10_log_mpidump

/*-- mpi-add.c --*/
void mpi_add_ui(MPI w, MPI u, ulong v );
void mpi_add(MPI w, MPI u, MPI v);
void mpi_addm(MPI w, MPI u, MPI v, MPI m);
void mpi_sub_ui(MPI w, MPI u, ulong v );
void mpi_sub( MPI w, MPI u, MPI v);
void mpi_subm( MPI w, MPI u, MPI v, MPI m);

/*-- mpi-mul.c --*/
void mpi_mul_ui(MPI w, MPI u, ulong v );
void mpi_mul_2exp( MPI w, MPI u, ulong cnt);
void mpi_mul( MPI w, MPI u, MPI v);
void mpi_mulm( MPI w, MPI u, MPI v, MPI m);

/*-- mpi-div.c --*/
ulong mpi_fdiv_r_ui( MPI rem, MPI dividend, ulong divisor );
void  mpi_fdiv_r( MPI rem, MPI dividend, MPI divisor );
void  mpi_fdiv_q( MPI quot, MPI dividend, MPI divisor );
void  mpi_fdiv_qr( MPI quot, MPI rem, MPI dividend, MPI divisor );
void  mpi_tdiv_r( MPI rem, MPI num, MPI den);
void  mpi_tdiv_qr( MPI quot, MPI rem, MPI num, MPI den);
void  mpi_tdiv_q_2exp( MPI w, MPI u, unsigned count );
int   mpi_divisible_ui(MPI dividend, ulong divisor );

/*-- mpi-gcd.c --*/
int mpi_gcd( MPI g, MPI a, MPI b );

/*-- mpi-pow.c --*/
void mpi_pow( MPI w, MPI u, MPI v);
void mpi_powm( MPI res, MPI base, MPI exp, MPI mod);

/*-- mpi-mpow.c --*/
void mpi_mulpowm( MPI res, MPI *basearray, MPI *exparray, MPI mod);

/*-- mpi-cmp.c --*/
int mpi_cmp_ui( MPI u, ulong v );
int mpi_cmp( MPI u, MPI v );

/*-- mpi-scan.c --*/
int mpi_getbyte( MPI a, unsigned idx );
void mpi_putbyte( MPI a, unsigned idx, int value );
unsigned mpi_trailing_zeros( MPI a );

/*-- mpi-bit.c --*/
void mpi_normalize( MPI a );
unsigned mpi_get_nbits( MPI a );
int  mpi_test_bit( MPI a, unsigned n );
void mpi_set_bit( MPI a, unsigned n );
void mpi_set_highbit( MPI a, unsigned n );
void mpi_clear_highbit( MPI a, unsigned n );
void mpi_clear_bit( MPI a, unsigned n );
void mpi_rshift( MPI x, MPI a, unsigned n );

/*-- mpi-inv.c --*/
void mpi_invm( MPI x, MPI u, MPI v );


#endif /*G10_MPI_H*/
