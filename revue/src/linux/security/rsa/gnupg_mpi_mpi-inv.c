/* mpi-inv.c  -  MPI functions
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "gnupg_mpi_mpi-internal.h"


/****************
 * Calculate the multiplicative inverse X of A mod N
 * That is: Find the solution x for
 *		1 = (a*x) mod n
 */
void
mpi_invm( MPI x, MPI a, MPI n )
{
  #if 0
    MPI u, v, u1, u2, u3, v1, v2, v3, q, t1, t2, t3;
    MPI ta, tb, tc;

    u = mpi_copy(a);
    v = mpi_copy(n);
    u1 = mpi_alloc_set_ui(1);
    u2 = mpi_alloc_set_ui(0);
    u3 = mpi_copy(u);
    v1 = mpi_alloc_set_ui(0);
    v2 = mpi_alloc_set_ui(1);
    v3 = mpi_copy(v);
    q  = mpi_alloc( mpi_get_nlimbs(u)+1 );
    t1 = mpi_alloc( mpi_get_nlimbs(u)+1 );
    t2 = mpi_alloc( mpi_get_nlimbs(u)+1 );
    t3 = mpi_alloc( mpi_get_nlimbs(u)+1 );
    while( mpi_cmp_ui( v3, 0 ) ) {
	mpi_fdiv_q( q, u3, v3 );
	mpi_mul(t1, v1, q); mpi_mul(t2, v2, q); mpi_mul(t3, v3, q);
	mpi_sub(t1, u1, t1); mpi_sub(t2, u2, t2); mpi_sub(t3, u3, t3);
	mpi_set(u1, v1); mpi_set(u2, v2); mpi_set(u3, v3);
	mpi_set(v1, t1); mpi_set(v2, t2); mpi_set(v3, t3);
    }
    /*	log_debug("result:\n");
	log_mpidump("q =", q );
	log_mpidump("u1=", u1);
	log_mpidump("u2=", u2);
	log_mpidump("u3=", u3);
	log_mpidump("v1=", v1);
	log_mpidump("v2=", v2); */
    mpi_set(x, u1);

    mpi_free(u1);
    mpi_free(u2);
    mpi_free(u3);
    mpi_free(v1);
    mpi_free(v2);
    mpi_free(v3);
    mpi_free(q);
    mpi_free(t1);
    mpi_free(t2);
    mpi_free(t3);
    mpi_free(u);
    mpi_free(v);
  #elif 0
    /* Extended Euclid's algorithm (See TAOPC Vol II, 4.5.2, Alg X)
     * modified according to Michael Penk's solution for Exercice 35 */

    /* FIXME: we can simplify this in most cases (see Knuth) */
    MPI u, v, u1, u2, u3, v1, v2, v3, t1, t2, t3;
    unsigned k;
    int sign;

    u = mpi_copy(a);
    v = mpi_copy(n);
    for(k=0; !mpi_test_bit(u,0) && !mpi_test_bit(v,0); k++ ) {
	mpi_rshift(u, u, 1);
	mpi_rshift(v, v, 1);
    }


    u1 = mpi_alloc_set_ui(1);
    u2 = mpi_alloc_set_ui(0);
    u3 = mpi_copy(u);
    v1 = mpi_copy(v);				   /* !-- used as const 1 */
    v2 = mpi_alloc( mpi_get_nlimbs(u) ); mpi_sub( v2, u1, u );
    v3 = mpi_copy(v);
    if( mpi_test_bit(u, 0) ) { /* u is odd */
	t1 = mpi_alloc_set_ui(0);
	t2 = mpi_alloc_set_ui(1); t2->sign = 1;
	t3 = mpi_copy(v); t3->sign = !t3->sign;
	goto Y4;
    }
    else {
	t1 = mpi_alloc_set_ui(1);
	t2 = mpi_alloc_set_ui(0);
	t3 = mpi_copy(u);
    }
    do {
	do {
	    if( mpi_test_bit(t1, 0) || mpi_test_bit(t2, 0) ) { /* one is odd */
		mpi_add(t1, t1, v);
		mpi_sub(t2, t2, u);
	    }
	    mpi_rshift(t1, t1, 1);
	    mpi_rshift(t2, t2, 1);
	    mpi_rshift(t3, t3, 1);
	  Y4:
	    ;
	} while( !mpi_test_bit( t3, 0 ) ); /* while t3 is even */

	if( !t3->sign ) {
	    mpi_set(u1, t1);
	    mpi_set(u2, t2);
	    mpi_set(u3, t3);
	}
	else {
	    mpi_sub(v1, v, t1);
	    sign = u->sign; u->sign = !u->sign;
	    mpi_sub(v2, u, t2);
	    u->sign = sign;
	    sign = t3->sign; t3->sign = !t3->sign;
	    mpi_set(v3, t3);
	    t3->sign = sign;
	}
	mpi_sub(t1, u1, v1);
	mpi_sub(t2, u2, v2);
	mpi_sub(t3, u3, v3);
	if( t1->sign ) {
	    mpi_add(t1, t1, v);
	    mpi_sub(t2, t2, u);
	}
    } while( mpi_cmp_ui( t3, 0 ) ); /* while t3 != 0 */
    /* mpi_lshift( u3, k ); */
    mpi_set(x, u1);

    mpi_free(u1);
    mpi_free(u2);
    mpi_free(u3);
    mpi_free(v1);
    mpi_free(v2);
    mpi_free(v3);
    mpi_free(t1);
    mpi_free(t2);
    mpi_free(t3);
  #else
    /* Extended Euclid's algorithm (See TAOPC Vol II, 4.5.2, Alg X)
     * modified according to Michael Penk's solution for Exercice 35
     * with further enhancement */
    MPI u, v, u1, u2=NULL, u3, v1, v2=NULL, v3, t1, t2=NULL, t3;
    unsigned k;
    int sign;
    int odd ;

    u = mpi_copy(a);
    v = mpi_copy(n);

    for(k=0; !mpi_test_bit(u,0) && !mpi_test_bit(v,0); k++ ) {
	mpi_rshift(u, u, 1);
	mpi_rshift(v, v, 1);
    }
    odd = mpi_test_bit(v,0);

    u1 = mpi_alloc_set_ui(1);
    if( !odd )
	u2 = mpi_alloc_set_ui(0);
    u3 = mpi_copy(u);
    v1 = mpi_copy(v);
    if( !odd ) {
	v2 = mpi_alloc( mpi_get_nlimbs(u) );
	mpi_sub( v2, u1, u ); /* U is used as const 1 */
    }
    v3 = mpi_copy(v);
    if( mpi_test_bit(u, 0) ) { /* u is odd */
	t1 = mpi_alloc_set_ui(0);
	if( !odd ) {
	    t2 = mpi_alloc_set_ui(1); t2->sign = 1;
	}
	t3 = mpi_copy(v); t3->sign = !t3->sign;
	goto Y4;
    }
    else {
	t1 = mpi_alloc_set_ui(1);
	if( !odd )
	    t2 = mpi_alloc_set_ui(0);
	t3 = mpi_copy(u);
    }
    do {
	do {
	    if( !odd ) {
		if( mpi_test_bit(t1, 0) || mpi_test_bit(t2, 0) ) { /* one is odd */
		    mpi_add(t1, t1, v);
		    mpi_sub(t2, t2, u);
		}
		mpi_rshift(t1, t1, 1);
		mpi_rshift(t2, t2, 1);
		mpi_rshift(t3, t3, 1);
	    }
	    else {
		if( mpi_test_bit(t1, 0) )
		    mpi_add(t1, t1, v);
		mpi_rshift(t1, t1, 1);
		mpi_rshift(t3, t3, 1);
	    }
	  Y4:
	    ;
	} while( !mpi_test_bit( t3, 0 ) ); /* while t3 is even */

	if( !t3->sign ) {
	    mpi_set(u1, t1);
	    if( !odd )
		mpi_set(u2, t2);
	    mpi_set(u3, t3);
	}
	else {
	    mpi_sub(v1, v, t1);
	    sign = u->sign; u->sign = !u->sign;
	    if( !odd )
		mpi_sub(v2, u, t2);
	    u->sign = sign;
	    sign = t3->sign; t3->sign = !t3->sign;
	    mpi_set(v3, t3);
	    t3->sign = sign;
	}
	mpi_sub(t1, u1, v1);
	if( !odd )
	    mpi_sub(t2, u2, v2);
	mpi_sub(t3, u3, v3);
	if( t1->sign ) {
	    mpi_add(t1, t1, v);
	    if( !odd )
		mpi_sub(t2, t2, u);
	}
    } while( mpi_cmp_ui( t3, 0 ) ); /* while t3 != 0 */
    /* mpi_lshift( u3, k ); */
    mpi_set(x, u1);

    mpi_free(u1);
    mpi_free(v1);
    mpi_free(t1);
    if( !odd ) {
	mpi_free(u2);
	mpi_free(v2);
	mpi_free(t2);
    }
    mpi_free(u3);
    mpi_free(v3);
    mpi_free(t3);

    mpi_free(u);
    mpi_free(v);
  #endif
}



