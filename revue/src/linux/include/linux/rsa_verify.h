

struct rsa_key {
	const char *n;
	int n_size;
	const char *e;
	int e_size;
};

extern int rsa_install_key(struct rsa_key *rsa_key);
extern int rsa_verify_sig(char *sig, int sig_size, u8 *sha1);
