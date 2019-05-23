#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "definitionsDES.h"
#include "functionsDES.c" //TODO Criar functionsDES.h com a cabecalho das funcoes e incluir aqui no lugar do .c
 
int obtain_key(key_t2 previous_key, key_t2 next_key) {
	//Alterar funcao para fornecer a proxima chave a ser testada a partir da chave anterior.
	//Retornar 1 se houver uma nova chave a ser testada e retorna 0 se nao houver.
	/*
	Processo de criacao de chave:
	1-Ultimos 16 bits (4 hexadecimais) formam a chave propriamente dita.
	2-Completar com 0's a esquerda ate formar 56 bits.
	3-Da esquerda para a direita, contar 7 bits. Se o numero de 1's for par, incluir um 8 bit "1". Se for impar, incluir um 8 bit "0".
	4-A chave formada de 16 bits deve ser escrita como um array de oito bytes, como feito abaixo para as mensagens.
	*/
	int i;
	for(i=0; i<KEY_LEN; i++) {
		printf("Valor de previous_key[%d]: %x\n", i, previous_key[i]);
		next_key[i] = previous_key[i];
		printf("Valor de next_key[%d]    : %x\n", i, next_key[i]);
	}
	return 0;
}

int main() {
	const ubyte messagePlain[] = { 0x43, 0x41, 0x4D, 0x49, 0x4E, 0x48, 0x4F, 0x53 };
	const ubyte messageEncoded[] = { 0x2B, 0xB1, 0x8A, 0xAE, 0xA2, 0xE6, 0x27, 0x8A };
	int len = sizeof(messagePlain) / sizeof(ubyte);
	int there_is_next_key = 1;
	int j;
	char buffer[128], bufferMessageEncoded[128];
	String encoded;

	//key_t2 key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
	key_t2 key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x07, 0xD5, 0x94 };

	while(there_is_next_key) { 
		key_t2 next_key;
		there_is_next_key = obtain_key(key, next_key);
		encoded = encrypt(key, messagePlain, len);
		printBytes(encoded.data, encoded.len/2, buffer);
		printBytes(messageEncoded, encoded.len/2, bufferMessageEncoded);

		if(!strcmp(buffer, bufferMessageEncoded)) {
    			printf("Encoded text : %s\n", buffer);

			printBytes(messagePlain, len, buffer);
			printf("Plain text   : %s\n", buffer);

			printBytes(key, KEY_LEN, buffer);
			printf("Key          : %s\n", buffer);

			/*printBytes(next_key, KEY_LEN, buffer);
                        printf("Next key     : %s\n", buffer);*/

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
