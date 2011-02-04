
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/rsa_verify.h>

#include "gnupg_mpi_mpi.h"


extern int rsa_verify(MPI hash, MPI *data, MPI *pkey);


static struct rsa_key *rsa_key;

int rsa_install_key(struct rsa_key *new_key)
{
	if (rsa_key) {
		return -EINVAL;
	}

	rsa_key = new_key;
	return 0;
}
EXPORT_SYMBOL(rsa_install_key);


int rsa_verify_sig(char *sig, int sig_size, u8 *sha1)
{
	MPI public_key[2];
	MPI hash;
	MPI data;
	int nread;
	int nframe;

	if (!rsa_key) {
		return -EINVAL;
	}

	/* initialize our keys */
	nread = rsa_key->n_size;
	public_key[0] = mpi_read_from_buffer((byte *)rsa_key->n, &nread, 0);
	if (!public_key[0]) {
		return -EINVAL;
	}

	nread = rsa_key->e_size;
	public_key[1] = mpi_read_from_buffer((byte *)rsa_key->e, &nread, 0);
	if (!public_key[1]) {
		return -EINVAL;
	}

	/* set up the MPI for the sig */
	data = mpi_read_from_buffer(sig, &sig_size, 0);
	if (!data) {
		return -EINVAL;
	}

	/* set up the MPI for the result */
	nframe = mpi_get_nbits(public_key[0]);
	hash = do_encode_md(sha1, nframe);

	/* verify the sig */
	return rsa_verify(hash, &data, public_key);
}
EXPORT_SYMBOL(rsa_verify_sig);



