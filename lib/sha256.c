#include <gc-helper.h>

typedef struct sha256_context {
	uint32_t H[64 + 8 + 8];
	uint8_t buf[64];
	size_t qbytes;
} Sha256Context;

void sha256_init(struct sha256_context * restrict ctx);
void sha256_update(struct sha256_context * restrict ctx, const void * data, size_t len);
void sha256_destroy(struct sha256_context * restrict ctx, void * restrict hash);



/* 32 bit cyclic rotations */

static inline uint32_t rotl(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static inline uint32_t rotr(uint32_t x, int n)
{
    return (x >> n) | (x << (32 - n));
}



/* SHA-256 magic funcs & consts */

static inline uint32_t ch(const uint32_t * v)
{
    uint32_t x = v[0];
    uint32_t y = v[1];
    uint32_t z = v[2];
    return (x & y) ^ (~x & z);
}

static inline uint32_t maj(const uint32_t * v)
{
    uint32_t x = v[0];
    uint32_t y = v[1];
    uint32_t z = v[2];
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint32_t Sigma0(uint32_t x)
{
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static inline uint32_t Sigma1(uint32_t x)
{
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static inline uint32_t sigma0(uint32_t x)
{
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static inline uint32_t sigma1(uint32_t x)
{
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t H0[8] = {
	0x6a09e667,	0xbb67ae85,	0x3c6ef372,	0xa54ff53a,	0x510e527f,	0x9b05688c,	0x1f83d9ab,	0x5be0cd19
};



/* SHA-256: processing a new 64 byte chunk */

static void sha256_process_chunk(struct sha256_context * restrict ctx, const void * data)
{
    uint32_t W[64]; // message's shedule

    /* We consider out stream as Big Endian bytes. */
    const uint32_t * data_as_32 = data;
    for (int i = 0; i < 16; ++i) {
        W[i] = be32toh(data_as_32[i]);
    }

    /* Compute the rest of the message's schedule */
	for (int i = 16; i < 64; ++i) {
		W[i] = 0
            + sigma1(W[i - 2])
            + W[i - 7]
            + sigma0(W[i - 15])
            + W[i - 16]
            ;
    }

    /* Put the initial hash value at a top */
    uint32_t * H = ctx->H + 64 + 8;
    memcpy(H, ctx->H, 32);

    /* Cyclic job */
	for (int i = 0; i < 64; ++i) {
        /* Caclulate T1 & T2 */
        const uint32_t a = H[0];
        const uint32_t e = H[4];
        const uint32_t h = H[7];

		const uint32_t T1 = h + Sigma1(e) + ch(H + 4) + K[i] + W[i];
		const uint32_t T2 = Sigma0(a) + maj(H + 0);

        /* Shift hash value down */
        H--;
        H[4] += T1;
        H[0] = T1 + T2;
	}

    /* Finish intermiate hash */
    for (int i=0; i<8; ++i) {
        ctx->H[i] += H[i];
    }
}



/* SHA-256: calculation context initialization */

void sha256_init(struct sha256_context * restrict ctx)
{
    /* No bytes, the initial hash value */
	ctx->qbytes = 0;
    memcpy(ctx->H, H0, 32);
}



/* SHA-256: eating a new data portion */

void sha256_update(struct sha256_context * restrict ctx, const void * data, size_t len)
{
    /* We use it for address ariphmetic */
    const char * ptr = data;

    /* Calculate amount of remaining bytes and increase a byte counter */
    size_t remaining = ctx->qbytes % 64;
    ctx->qbytes += len;

    if (remaining > 0) {
        /* Something remaining from the last update, we must to use it */

        if (len + remaining < 64) {
            /* To small data portion, chunk processing is not needed */
            memcpy(ctx->buf + remaining, ptr, len);
            return;
        }

        /* Process chunk which contains remaining and new data */
        size_t needed = 64 - remaining;
        memcpy(ctx->buf + remaining, ptr, needed);
        sha256_process_chunk(ctx, ctx->buf);
        ptr += needed;
        len -= needed;
    }

    /* Now we can process chunks inplace without copying */
    while (len >= 64) {
        sha256_process_chunk(ctx, ptr);
        ptr += 64;
        len -= 64;
    }

    /* And we must save remaining for future usage */
    if (len > 0) {
        memcpy(ctx->buf, ptr, len);
    }
}



/* SHA-256 asking for a hash value */

void sha256_destroy(struct sha256_context * restrict ctx, void * restrict hash)
{
    /* Calculate amount of remaining bytes and increase a byte counter */
    size_t remaining = ctx->qbytes % 64;
    uint64_t qbits = 8 * ctx->qbytes;

    /* Write terminator, it is always possible and it is always needed */
    ctx->buf[remaining++] = 0x80;

    /* Is it enought space to save qbits? */
	if (remaining <= 56) {
        /* Yes, it is! Only pad with zeroes if needed */
        size_t qzeros = 56 - remaining;
        memset(ctx->buf + remaining, 0, qzeros);
	}
	else {
        /* No, it is not! Pad with zeroes, process chunk and pad with zeroes a new chunk */
        size_t qzeros = 64 - remaining;
        memset(ctx->buf + remaining, 0, qzeros);

		sha256_process_chunk(ctx, ctx->buf);
		memset(ctx->buf, 0, 56);
	}

	/* Now 8 bytes left in the last chunk. Write qbits and process */
    uint64_t * qbits_ptr = (void*)(ctx->buf + 56);
    *qbits_ptr = htobe64(qbits);
	sha256_process_chunk(ctx, ctx->buf);

    /* Convert to Big Endian byte order */
    uint32_t * restrict hash_as_u32 = hash;
    for (int i=0; i < 8; ++i) {
        hash_as_u32[i] = htobe32(ctx->H[i]);
    }
}





typedef struct sha256_context_old {
	uint8_t data[64];
	uint32_t qbytes;
	uint32_t state[8];
	uint64_t qbits;
} Sha256Context_Old;

void sha256_init_old(struct sha256_context_old * ctx);
void sha256_update_old(struct sha256_context_old * ctx, const uint8_t * data, size_t len);
void sha256_destroy_old(struct sha256_context_old * ctx, uint8_t * restrict hash);

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

void sha256_transform_old(struct sha256_context_old * restrict ctx, const uint8_t * data)
{
	uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + K[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);

		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init_old(struct sha256_context_old *ctx)
{
	ctx->qbytes = 0;
	ctx->qbits = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void sha256_update_old(struct sha256_context_old *ctx, const uint8_t data[], size_t len)
{
	uint32_t i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->qbytes] = data[i];
		ctx->qbytes++;
		if (ctx->qbytes == 64) {
			sha256_transform_old(ctx, ctx->data);
			ctx->qbits += 512;
			ctx->qbytes = 0;
		}
	}
}

void sha256_destroy_old(struct sha256_context_old *ctx, uint8_t hash[])
{

	uint32_t i;

	i = ctx->qbytes;

	// Pad whatever data is left in the buffer.
	if (ctx->qbytes < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform_old(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	// Append to the padding the total message's length in bits and transform.
	ctx->qbits += (uint64_t)ctx->qbytes * 8;

	ctx->data[63] = ctx->qbits;
	ctx->data[62] = ctx->qbits >> 8;
	ctx->data[61] = ctx->qbits >> 16;
	ctx->data[60] = ctx->qbits >> 24;
	ctx->data[59] = ctx->qbits >> 32;
	ctx->data[58] = ctx->qbits >> 40;
	ctx->data[57] = ctx->qbits >> 48;
	ctx->data[56] = ctx->qbits >> 56;
	sha256_transform_old(ctx, ctx->data);

	// Since this implementation uses little endian byte ordering and SHA uses big endian,
	// reverse all the bytes when copying the final state to the output hash.
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}



#include <stdio.h>
#include <stdlib.h>

void * read_file(FILE * f, long * sz)
{
    int status;

    status = fseek(f, 0, SEEK_END);
    if (status != 0) {
        printf("fseek failed with code %d\n", status);
        return NULL;
    }

    *sz = ftell(f);
    if (*sz == 0) {
        return NULL;
    }

    if (*sz < 0) {
        printf("ftell returned %ld\n", *sz);
        return NULL;
    }

    rewind(f);

    void * data = malloc(*sz);
    if (data == NULL) {
        printf("malloc returned NULL for size %ld\n", *sz);
        return NULL;
    }

    size_t was_read = fread(data, 1, *sz, f);
    if (was_read != *sz) {
        printf("fread did not read enought %" PRIu64 "\n", was_read);
        free(data);
        return NULL;
    }

    return data;
}

int check_sha256(const void * data, int len)
{
    char hash[32];
    const void * tail = (const char *)data + len;

    struct sha256_context context;
    struct sha256_context * restrict ctx = &context;

    sha256_init(ctx);
    sha256_update(ctx, data, len);
    sha256_destroy(ctx, hash);

    return memcmp(hash, tail, 32);
}

void write_sha256(void * restrict data, int len)
{
    void * restrict tail = (char *)data + len;

    struct sha256_context context;
    struct sha256_context * restrict ctx = &context;

    sha256_init(ctx);
    sha256_update(ctx, data, len);
    sha256_destroy(ctx, tail);
}
