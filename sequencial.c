#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "definitionsDES.h"
 
typedef struct {
    ubyte *data;
    int len;
} String;
 
/*
 * Transform a single nibble into a hex character
 *
 * in: a value < 0x10
 *
 * returns: the character that represents the nibble
 */
static char toHex(ubyte in) {
    if (0x00 <= in && in < 0x0A) {
        return '0' + in;
    }
    if (0x0A <= in && in <= 0x0F) {
        return 'A' + in - 0x0A;
    }
    return 0;
}
 
/*
 * Convert an array of bytes into a string
 *
 * ptr: the array of bytes
 * len: the number of bytes
 * out: a buffer allocated by the caller with enough space for 2*len+1 characters
 */
static void printBytes(const ubyte *ptr, int len, char *out) {
    while (len-- > 0) {
        *out++ = toHex(*ptr >> 4);
        *out++ = toHex(*ptr & 0x0F);
 
        ptr++;
    }
    *out = 0;
}
 
/*
 * Gets the value of a bit in an array of bytes
 *
 * src: the array of bytes to index
 * index: the desired bit to test the value of
 *
 * returns: the bit at the specified position in the array
 */
static int peekBit(const ubyte *src, int index) {
    int cell = index / 8;
    int bit = 7 - index % 8;
    return (src[cell] & (1 << bit)) != 0;
}
 
/*
 * Sets the value of a bit in an array of bytes
 *
 * dst: the array of bits to set a bit in
 * index: the position of the bit to set
 * value: the value for the bit to set
 */
static void pokeBit(ubyte *dst, int index, int value) {
    int cell = index / 8;
    int bit = 7 - index % 8;
    if (value == 0) {
        dst[cell] &= ~(1 << bit);
    } else {
        dst[cell] |= (1 << bit);
    }
}
 
/*
 * Transforms one array of bytes by shifting the bits the specified number of positions
 *
 * src: the array to shift bits from
 * len: the length of the src array
 * times: the number of positions that the bits should be shifted
 * dst: a bytes array allocated by the caller to store the shifted values
 */
static void shiftLeft(const ubyte *src, int len, int times, ubyte *dst) {
    int i, t;
    for (i = 0; i <= len; ++i) {
        pokeBit(dst, i, peekBit(src, i));
    }
    for (t = 1; t <= times; ++t) {
        int temp = peekBit(dst, 0);
        for (i = 1; i <= len; ++i) {
            pokeBit(dst, i - 1, peekBit(dst, i));
        }
        pokeBit(dst, len - 1, temp);
    }
}
 
/*
 * Calculates the sub keys to be used in processing the messages
 *
 * key: the array of bytes representing the key
 * ks: the subkeys that have been allocated by the caller
 */
typedef ubyte subkey_t[17][6]; /* 17 sets of 48 bits */
static void getSubKeys(const key_t2 key, subkey_t ks) {
    ubyte c[17][7];  /* 56 bits */
    ubyte d[17][4];  /* 28 bits */
    ubyte kp[7];
    int i, j;
 
    /* intialize */
    memset(c, 0, sizeof(c));
    memset(d, 0, sizeof(d));
    memset(ks, 0, sizeof(subkey_t));
 
    /* permute 'key' using table PC1 */
    for (i = 0; i < 56; ++i) {
        pokeBit(kp, i, peekBit(key, PC1[i] - 1));
    }
 
    /* split 'kp' in half and process the resulting series of 'c' and 'd' */
    for (i = 0; i < 28; ++i) {
        pokeBit(c[0], i, peekBit(kp, i));
        pokeBit(d[0], i, peekBit(kp, i + 28));
    }
 
    /* shift the components of c and d */
    for (i = 1; i < 17; ++i) {
        shiftLeft(c[i - 1], 28, SHIFTS[i - 1], c[i]);
        shiftLeft(d[i - 1], 28, SHIFTS[i - 1], d[i]);
    }
 
    /* merge 'd' into 'c' */
    for (i = 1; i < 17; ++i) {
        for (j = 28; j < 56; ++j) {
            pokeBit(c[i], j, peekBit(d[i], j - 28));
        }
    }
 
    /* form the sub-keys and store them in 'ks'
     * permute 'c' using table PC2 */
    for (i = 1; i < 17; ++i) {
        for (j = 0; j < 48; ++j) {
            pokeBit(ks[i], j, peekBit(c[i], PC2[j] - 1));
        }
    }
}
 
/*
 * Function used in processing the messages
 *
 * r: an array of bytes to be processed
 * ks: one of the subkeys to be used for processing
 * sp: output from the processing
 */
static void f(ubyte *r, ubyte *ks, ubyte *sp) {
    ubyte er[6]; /* 48 bits */
    ubyte sr[4]; /* 32 bits */
    int i;
 
    /* initialize */
    memset(er, 0, sizeof(er));
    memset(sr, 0, sizeof(sr));
 
    /* permute 'r' using table E */
    for (i = 0; i < 48; ++i) {
        pokeBit(er, i, peekBit(r, E[i] - 1));
    }
 
    /* xor 'er' with 'ks' and store back into 'er' */
    for (i = 0; i < 6; ++i) {
        er[i] ^= ks[i];
    }
 
    /* process 'er' six bits at a time and store resulting four bits in 'sr' */
    for (i = 0; i < 8; ++i) {
        int j = i * 6;
        int b[6];
        int k, row, col, m, n;
 
        for (k = 0; k < 6; ++k) {
            b[k] = peekBit(er, j + k) != 0 ? 1 : 0;
        }
 
        row = 2 * b[0] + b[5];
        col = 8 * b[1] + 4 * b[2] + 2 * b[3] + b[4];
        m = S[i][row * 16 + col]; /* apply table s */
        n = 1;
 
        while (m > 0) {
            int p = m % 2;
            pokeBit(sr, (i + 1) * 4 - n, p == 1);
            m /= 2;
            n++;
        }
    }
 
    /* permute sr using table P */
    for (i = 0; i < 32; ++i) {
        pokeBit(sp, i, peekBit(sr, P[i] - 1));
    }
}
 
/*
 * Processing of block of the message
 *
 * message: an 8 byte block from the message
 * ks: the subkeys to use in processing
 * ep: space for an encoded 8 byte block allocated by the caller
 */
static void processMessage(const ubyte *message, subkey_t ks, ubyte *ep) {
    ubyte left[17][4];  /* 32 bits */
    ubyte right[17][4]; /* 32 bits */
    ubyte mp[8];        /* 64 bits */
    ubyte e[8];         /* 64 bits */
    int i, j;
 
    /* permute 'message' using table IP */
    for (i = 0; i < 64; ++i) {
        pokeBit(mp, i, peekBit(message, IP[i] - 1));
    }
 
    /* split 'mp' in half and process the resulting series of 'l' and 'r */
    for (i = 0; i < 32; ++i) {
        pokeBit(left[0], i, peekBit(mp, i));
        pokeBit(right[0], i, peekBit(mp, i + 32));
    }
    for (i = 1; i < 17; ++i) {
        ubyte fs[4]; /* 32 bits */
 
        memcpy(left[i], right[i - 1], 4);
        f(right[i - 1], ks[i], fs);
        for (j = 0; j < 4; ++j) {
            left[i - 1][j] ^= fs[j];
        }
        memcpy(right[i], left[i - 1], 4);
    }
 
    /* amalgamate r[16] and l[16] (in that order) into 'e' */
    for (i = 0; i < 32; ++i) {
        pokeBit(e, i, peekBit(right[16], i));
    }
    for (i = 32; i < 64; ++i) {
        pokeBit(e, i, peekBit(left[16], i - 32));
    }
 
    /* permute 'e' using table IP2 ad return result as a hex string */
    for (i = 0; i < 64; ++i) {
        pokeBit(ep, i, peekBit(e, IP2[i] - 1));
    }
}
 
/*
 * Encrypts a message using DES
 *
 * key: the key to use to encrypt the message
 * message: the message to be encrypted
 * len: the length of the message
 *
 * returns: a paring of dynamically allocated memory for the encoded message,
 *          and the length of the encoded message.
 *          the caller will need to free the memory after use.
 */
String encrypt(const key_t2 key, const ubyte *message, int len) {
    String result = { 0, 0 };
    subkey_t ks;
    ubyte padByte;
    int i;
 
    getSubKeys(key, ks);
 
    padByte = 8 - len % 8;
    result.len = len + padByte;
    result.data = (ubyte*)malloc(result.len);
    memcpy(result.data, message, len);
    memset(&result.data[len], padByte, padByte);
 
    for (i = 0; i < result.len; i += 8) {
        processMessage(&result.data[i], ks, &result.data[i]);
    }
 
    return result;
}

int main() {
	const ubyte messagePlain[] = { 0x43, 0x41, 0x4D, 0x49, 0x4E, 0x48, 0x4F, 0x53 };
	const ubyte messageEncoded[] = { 0x2B, 0xB1, 0x8A, 0xAE, 0xA2, 0xE6, 0x27, 0x8A };
	int len = sizeof(messagePlain) / sizeof(ubyte);
	int i;
	char buffer[128], bufferMessageEncoded[128];
	String encoded;

	int tamanho;
	printf("Entre com o tamanho da chave: ");
	scanf("%d", &tamanho);

	for(i=0; i<pow(2, tamanho); i++) { //Alterar para testar as pow(2, 16) chaves possiveis para chaves de 16 bits.
		//key_t2 key = *obtain_key(i); //Criar funcao para fornecer a chave "i" a ser testada. Apagar linha abaixo.
		key_t2 key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x07, 0xD5, 0x94 };
		encoded = encrypt(key, messagePlain, len);
		printBytes(encoded.data, encoded.len/2, buffer);
		printBytes(messageEncoded, encoded.len/2, bufferMessageEncoded);

		if(!strcmp(buffer, bufferMessageEncoded)) {
    			printf("Encoded text : %s\n", buffer);

			printBytes(messagePlain, len, buffer);
			printf("Plain text   : %s\n", buffer);

			printBytes(key, KEY_LEN, buffer);
			printf("Key          : %s\n", buffer);

			return 0;
		}
		if (encoded.len > 0) {
			free(encoded.data);
			encoded.data = 0;
		}

	};
	
	printf("Key not founded.");
	return 0;
}
