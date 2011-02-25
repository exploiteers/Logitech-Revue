/*
 * Freescale QUICC Engine USB Host Controller Driver
 *
 * Copyright (c) Freescale Semicondutor, Inc. 2006.
 *               Shlomi Gridish <gridish@freescale.com>
 *               Jerry Huang <Chang-Ming.Huang@freescale.com>
 * Copyright (c) Logic Product Development, Inc. 2007
 *               Peter Barada <peterb@logicpd.com>
 * Copyright (c) MontaVista Software, Inc. 2008.
 *               Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/* circular queue structure */
struct cir_q {
       int max;		/* size of queue */
       int max_in;	/* max items in queue */
       int first;	/* index of first in queue */
       int last;	/* index after last in queue */
       int read;	/* current reading position */
       void *table[1];	/* fake size */
};

/* circular queue handle */
static int cq_howmany(struct cir_q *cq)
{
	int l = cq->last;
	int f = cq->first;

	return l >= f ? l - f : l + cq->max - f;
}

static struct cir_q *cq_new(int size)
{
	struct cir_q *cq;

	cq = kzalloc((sizeof(*cq) + size * sizeof(void *)), GFP_KERNEL);
	if (cq) {
		cq->max = size;
		cq->first = 0;
		cq->last = 0;
		cq->read = 0;
		cq->max_in = 0;
	}

	return cq;
}

static void cq_delete(struct cir_q *cq)
{
	kfree(cq);
}

static int cq_put(struct cir_q *cq, void *p)
{
	int n;
	int k;

	/* see if we can freely advance the last pointer */
	n = cq->last;
	k = cq_howmany(cq);
	if ((k + 1) >= cq->max)
		return -1;

	if (++n >= cq->max)
		n = 0;

	/* add element to queue */
	cq->table[cq->last] = p;
	cq->last = n;
	if ((k + 1) > cq->max_in)
		cq->max_in = k + 1;

	return k;
}

static void *cq_get(struct cir_q *cq)
{
	int n;
	int k;
	void *p;

	n = cq->first;
	/* see if the queue is not empty */
	if (n == cq->last)
		return NULL;

	p = cq->table[n];
	if (++n >= cq->max)
		n = 0;
	if (cq->read == cq->first)
		cq->read = n;
	cq->first = n;

	/* see if we've passed our previous maximum */
	k = cq_howmany(cq);
	if (k > cq->max_in)
		cq->max_in = k;

	return p;
}
