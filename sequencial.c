#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "definitionsDES.h"
#include "functionsDES.c" //TODO Criar functionsDES.h com a cabecalho das funcoes e incluir aqui no lugar do .c

#define MESSAGE_LEN 8

ubyte bits_to_byte(int *bits, int pos_i) {
	int hex1, hex2;
	ubyte res;

	hex1 = 8*bits[pos_i] + 4*bits[pos_i+1] + 2*bits[pos_i+2] + bits[pos_i+3];
	hex2 = 8*bits[pos_i+4] + 4*bits[pos_i+5] + 2*bits[pos_i+6] + bits[pos_i+7];
	res = (ubyte)(hex1*16 + hex2);

	return res;
}
 
int obtain_key(key_t2 previous_key, key_t2 next_key) {
	/*
	Funcao fornece a proxima chave a ser testada a partir da chave anterior.
	Retornar 1 se houver uma nova chave a ser testada e retorna 0 se nao houver.
		Processo de criacao de chave:
	1-Ultimos 16 bits (4 hexadecimais) formam a chave propriamente dita.
	2-Completar com 0's a esquerda ate formar 56 bits.
	3-Da esquerda para a direita, contar 7 bits. Se o numero de 1's for par, incluir um 8 bit "1". Se for impar, incluir um 8 bit "0".
	4-A chave formada de 16 bits deve ser escrita como um array de oito bytes, como feito abaixo para as mensagens.
	*/
	int i;
	ubyte byte1, byte2, byte3;
	int bits_aux[16], bits[64];
	int j=0;

	byte1 = previous_key[5]; //Pegar bits 2, 1
	for (i = 2; i != 0; i--) {
		bits_aux[j] = (byte1 & (1 << i)) != 0;
		j++;
	}

	byte2 = previous_key[6]; //Pegar bits 7, 6, 5, 4, 3, 2, 1
	for (i = 7; i != 0; i--) {
		bits_aux[j] = (byte2 & (1 << i)) != 0;
                j++;
        }

	byte3 = previous_key[7]; //Pegar bits 7, 6, 5, 4, 3, 2, 1
	for (i = 7; i != 0; i--) {
		bits_aux[j] = (byte3 & (1 << i)) != 0;
                j++;
        }

	//Verificar se todos os bits sao 1. Se forem, retornar 1 pois nao ha proxima chave.
	for(i=0; i<16; i++) {
		if(bits_aux[i] == 0) break;
		if(i == 15) return 0;
	}

	/*printf("Valores de bits_aux[16] antes:\n");
        for(i=0; i<16;i++) printf("%d", bits_aux[i]);
        printf("\n");*/

	//Obter proxima chave de 16 bits.
	for(i=15; i>=0; i--) {
		if(bits_aux[i] == 0) {
			bits_aux[i]++;
			break;
		} else {
			bits_aux[i]--;
		}
	}

	/*printf("Valores de bits_aux[16] depois:\n");
	for(i=0; i<16;i++) printf("%d", bits_aux[i]);
	printf("\n");*/

	//Juntar os 16 bits e formar 2 bytes (um numero hexadecimal). Obter proximo numero hexadecimal.
	//A partir dessa chave de 16 bits, obter a chave de 64 bits "estufando".
	for(i=0; i<45; i++) {
		if(i%8 == 7) {
			bits[i] = 1;
		} else {
			bits[i] = 0;
		}
	}

	j=0;

	for(i=45; i<47; i++) {
		bits[i] = bits_aux[j];
		j++;
	}
	int count_one = 0;
	for(i=0; i<2; i++) {
		if(bits_aux[i] == 1) count_one++;
	}
	if(count_one%2 == 0) bits[47] = 1;
	else bits[47] = 0;

	for(i=48; i<55; i++) {
		bits[i] = bits_aux[j];
		j++;
	}
	count_one = 0;
	for(i=2; i<9; i++) {
		if(bits_aux[i] == 1) count_one++;
	}
	if(count_one%2 == 0) bits[55] = 1;
        else bits[55] = 0;

	for(i=56; i<63; i++) {
		bits[i] = bits_aux[j];
		j++;
	}
	count_one = 0;
	for(i=9; i<16; i++) {
                if(bits_aux[i] == 1) count_one++;
        }
	if(count_one%2 == 0) bits[63] = 1;
        else bits[63] = 0;

	/*printf("Valor da nova chave com 64 bits:\n");
	for(i=0; i<64; i++) {
		printf("%d", bits[i]);
	}
	printf("\n");*/

	//Transformar o array de bits[64] no formato key_t2 em next_key.
	for(i=0; i<KEY_LEN; i++) {
		next_key[i] = bits_to_byte(bits, 8*i); //Funcao que pega um array de bits e a posicao inicial e transforma as oito posicoes do array a partir dessa inicial em um byte (retorna unsigned char - ubyte).
	}

	/*for(i=0; i<KEY_LEN; i++) {
		printf("Valor de previous_key[%d]: %x\n", i, previous_key[i]);
	}
	for(i=0; i<KEY_LEN; i++) {
                printf("Valor de next_key[%d]    : %x\n", i, next_key[i]);
        }*/

	return 1;
}

int main() {
	const ubyte messagePlain[] = { 0x43, 0x41, 0x4D, 0x49, 0x4E, 0x48, 0x4F, 0x53 };
	const ubyte messageEncoded[] = { 0x2B, 0xB1, 0x8A, 0xAE, 0xA2, 0xE6, 0x27, 0x8A };
	int len = sizeof(messagePlain) / sizeof(ubyte);
	int there_is_next_key = 1;
	int j;
	char buffer[128], bufferMessageEncoded[128];
	String encoded;
	printBytes(messageEncoded, MESSAGE_LEN, bufferMessageEncoded);

	//Primeira chave a ser testada
	key_t2 key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
	//key_t2 key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x07, 0xD5, 0x94 };

	while(there_is_next_key) { 
		key_t2 next_key;
		there_is_next_key = obtain_key(key, next_key);
		encoded = encrypt(key, messagePlain, len);
		printBytes(encoded.data, MESSAGE_LEN, buffer);

		if(!strcmp(buffer, bufferMessageEncoded)) {
    			printf("Encoded text : %s\n", buffer);

			printBytes(messagePlain, len, buffer);
			printf("Plain text   : %s\n", buffer);

			printBytes(key, KEY_LEN, buffer);
			printf("Key          : %s\n", buffer);

			printBytes(next_key, KEY_LEN, buffer);
                        printf("Next key     : %s\n", buffer);

			return 0;
		}
		if (encoded.len > 0) {
			free(encoded.data);
			encoded.data = 0;
		}
		for(j=0; j<KEY_LEN; j++) {
			key[j] = next_key[j];
		}
	};
	
	printf("Key not founded.");
	return 0;
}
