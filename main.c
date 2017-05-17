/* ###################################################################
 **     Filename    : main.c
 **     Project     : Datalogger
 **     Processor   : MKL25Z128VLK4
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2017-05-11, 14:50, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 01.01
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */         
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "EE241.h"
#include "WAIT1.h"
#include "GI2C1.h"
#include "C123.h"
#include "BitsIoLdd1.h"
#include "L1234.h"
#include "BitsIoLdd2.h"
#include "UART.h"
#include "ASerialLdd1.h"
#include "KSDK1.h"
#include "AD1.h"
#include "AdcLdd1.h"
#include "UTIL1.h"
#include "CLS1.h"
#include "CS1.h"
#include "CI2C1.h"
#include "REDLED.h"
#include "BitIoLdd1.h"
#include "WAIT2.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "TU1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static uint8_t value;
/* Flags globais para controle de processos da interrupcao */
volatile int flag_check_command = 0;
extern int rx;
extern int meas;

/* User includes (#include below this line is not maintained by Processor Expert) */

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
/* Rotina auxiliar para comparacao de strings */

int str_cmp(char *s1, char *s2, int len) {
	/* Compare two strings up to length len. Return 1 if they are equal, and 0 otherwise. */
	int i;
	for (i=0; i<len; i++) {
		if (s1[i] != s2[i]) return 0;
		if (s1[i] == '\0') return 1;
	}
	return 1;
}

/* Processo de bufferizacao. Caracteres recebidos sao armazenados em um buffer. Quando um caractere
 *  de fim de linha ('\n') e recebido, todos os caracteres do buffer sao processados simultaneamente.
 */

/* Buffer de dados recebidos */
#define MAX_BUFFER_SIZE 100
typedef struct {
	char data[MAX_BUFFER_SIZE];
	unsigned int tam_buffer;
} serial_buffer;

/* Teremos somente um buffer em nosso programa, O modificador volatile
 *  informa ao compilador que o conteudo de Buffer pode ser modificado a qualquer momento. Isso
 *  restringe algumas otimizacoes que o compilador possa fazer, evitando inconsistencias em
 *  algumas situacoes (por exemplo, evitando que ele possa ser modificado em uma rotina de interrupcao
 *  enquanto esta sendo lido no programa principal).
 */
volatile serial_buffer Buffer;

/* Todas as funcoes a seguir assumem que existe somente um buffer no programa e que ele foi
 *  declarado como Buffer. Esse padrao de design - assumir que so existe uma instancia de uma
 *  determinada estrutura - se chama Singleton (ou: uma adaptacao dele para a programacao
 *  nao-orientada-a-objetos). Ele evita que tenhamos que passar o endereco do
 *  buffer como parametro em todas as operacoes (isso pode economizar algumas instrucoes PUSH/POP
 *  nas chamadas de funcao, mas esse nao eh o nosso motivo principal para utiliza-lo), alem de
 *  garantir um ponto de acesso global a todas as informacoes contidas nele.
 */

/* Limpa buffer */
void buffer_clean() {
	Buffer.tam_buffer = 0;
}

/* Adiciona caractere ao buffer */
int buffer_add(char c_in) {
	if (Buffer.tam_buffer < MAX_BUFFER_SIZE) {
		Buffer.data[Buffer.tam_buffer] = c_in;
		Buffer.tam_buffer++;
		return 1;
	}
	return 0;
}

void Matrix() {
	L1234_ClrBit(0);
	if(!C123_GetBit(0)) {
		// Botão 1 apertado.
		buffer_add('1');
		WAIT1_Waitms(200); // Debouncing do botão para o teclado matricial.
	}
	if(!C123_GetBit(1)) {
		// Botão 2 apertado.
		buffer_add('2');
		WAIT1_Waitms(200);
	}
	if(!C123_GetBit(2)) {
		// Botão 3 apertado.
		buffer_add('3');
		WAIT1_Waitms(200);
	}
	L1234_SetBit(0);

	L1234_ClrBit(1);
	if(!C123_GetBit(0)) {
		// Botão 4 apertado.
		buffer_add('4');
		WAIT1_Waitms(200);
	}
	if(!C123_GetBit(1)) {
		buffer_add('5');
		WAIT1_Waitms(200);
		// Botão 5 apertado. Nada acontece.
	}
	if(!C123_GetBit(2)) {
		buffer_add('6');
		WAIT1_Waitms(200);
		// Botão 6 apertado. Nada acontece.
	}
	L1234_SetBit(1);

	L1234_ClrBit(2);
	if(!C123_GetBit(0)) {
		buffer_add('7');
		WAIT1_Waitms(200);
		// Botão 7 apertado. Nada acontece.
	}
	if(!C123_GetBit(1)) {
		buffer_add('8');
		WAIT1_Waitms(200);
		// Botão 8 apertado. Nada acontece.
	}
	if(!C123_GetBit(2)) {
		buffer_add('9');
		WAIT1_Waitms(200);
		// Botão 9 apertado. Nada acontece.
	}
	L1234_SetBit(2);

	L1234_ClrBit(3);
	if(!C123_GetBit(0)) {
		// Botão * apertado.
		flag_check_command = 1;
		buffer_add('*');
		WAIT1_Waitms(100);
		buffer_add('\0');
		WAIT1_Waitms(200);
	}
	if(!C123_GetBit(1)) {
		// Botão 0 apertado. Nada acontece.
	}
	if(!C123_GetBit(2)) {
		// Botão # apertado.
		buffer_add('#');
		WAIT1_Waitms(200);
	}
	L1234_SetBit(3);
}

int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
	/* Write your local variable definition here */
	buffer_clean();
	flag_check_command = 0;
	int i = 0, flag_write = 0, autoMeasure = 0, addr = 0, N=0, numsize=0, aux=0, cont=0;
	char out_buffer[30];
	uint8_t out_buffer_auto[16];
	unsigned char c;

	/*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
	PE_low_level_init();
	/*** End of Processor Expert internal initialization.                    ***/

	/* Write your code here */
	/* For example: for(;;) { } */

	for(;;) {
		
		(void)AD1_Measure(TRUE);
		(void)AD1_GetValue8(&value);
		
		Matrix();

		/* Ao receber evento da UART */
		if(rx) {
			// Caso não tenham havido problemas na recepção do caracter.
			if(UART_RecvChar(&c) == ERR_OK) {
				if (c =='\r') {
					buffer_add('\0'); /* Se recebeu um fim de linha, coloca um terminador de string no buffer */
					UART_SendChar('\n');
					// Considerando o atraso entro os ciclos da MCU e da comunicação com a UART torna-se
					// necessário o uso de uma função de delay.
					WAIT1_Waitms(50);
					UART_SendChar('\r');
					WAIT1_Waitms(50);
					flag_check_command = 1;
				} 
				else {
					buffer_add(c); // Guarda caracter recebido no buffer.
					UART_SendChar(c); // Eco para melhor visualização.
				}
			}
			rx = 0;
		}

		/* A flag_check_command permite separar a recepcao de caracteres
		 *  (vinculada a interrupcao) da interpretacao de caracteres. Dessa forma,
		 *  mantemos a rotina de interrupcao mais enxuta, enquanto o processo de
		 *  interpretacao de comandos - mais lento - nao impede a recepcao de
		 *  outros caracteres. Como o processo nao 'prende' a maquina, ele e chamado
		 *  de nao-preemptivo.
		 */
		if (flag_check_command == 1) {
			// Funções para a UART.
			if (str_cmp(Buffer.data, "PING", 4) ) {
				sprintf(out_buffer, "PONG\r\n");
				flag_write = 1;
			}
			else if (str_cmp(Buffer.data, "ID", 2) ) {
				// Retorna a string de identificação do projeto.
				sprintf(out_buffer, "DATALOGGER DO GRUPO 7\r\n");
				flag_write = 1;
			}
			else if (str_cmp(Buffer.data, "MEASURE", 7) ) {
				// Pega o valor de uma medição do LDR e prepara para impressão na UART.
				aux = (int)value;
				sprintf(out_buffer, "%d\n\r", aux);
				flag_write = 1;
			}
			else if (str_cmp(Buffer.data, "MEMSTATUS", 9) ) {
				// Pega o valor atual do ponteiro de endereços e prepara para impressão na UART.
				sprintf(out_buffer, "%d\n\r", addr);
				flag_write = 1;
			}
			else if (str_cmp(Buffer.data, "RESET", 5) ) {
				// Reinicializa o ponteiro de endereços da memória EEPROM.
				addr = 0;
			}
			else if (str_cmp(Buffer.data, "RECORD", 6) ) {
				// Pega uma medida do LDR e guarda seu valor na memória.
				if(addr < 16000){
					// Caso a memória ainda tenha espaço disponível.
					EE241_WriteByte(addr, value);
					addr++;
				}
				else {
					// Caso a memória não tenha mais espaço disponível.
					sprintf(out_buffer, "Memória Cheia!");
					flag_write = 1;
				}
			}
			else if (str_cmp(Buffer.data, "GET ", 4) ) {
				// Realiza o acesso a uma posição aleatória (N) de memória.
				numsize = strlen(Buffer.data) - 4; // Desconsidera primeiras 4 posições fixas do buffer de entrada.
				for(i=0;i<numsize;i++) {
					N += pow(10,(numsize-i-1))*(Buffer.data[i+4]-'0');
				}
				if (N<addr) {
					// Caso o valor dado como entrada esteja em uma posição válida de memória.
					EE241_ReadByte(N, &aux);
					sprintf(out_buffer, "%d\n\r", aux);
				}
				else {
					// Caso a posição acessada seja inválida.
					sprintf(out_buffer, "Posicao de memoria invalida\r\n");
				}
				N=0;
				flag_write = 1;
			}

			// Funções para o teclado matricial.
			else if(str_cmp(Buffer.data, "#1*", 3) ) {
				// Realiza o toggle do bit do pino ptb18 (LED vermelho).
				REDLED_NegVal();
				WAIT1_Waitms(300);
				REDLED_NegVal();
			}
			else if(str_cmp(Buffer.data, "#2*", 3) ) {
				if(addr < 16000){
					// Caso a posição de memória seja válida, realiza o armazenamento de uma leitura do LDR na EEPROM.
					EE241_WriteByte(addr, value);
					addr++;
				}
				else {
					sprintf(out_buffer, "Memoria Cheia!\r\n");
					flag_write = 1;
				}
			}
			else if(str_cmp(Buffer.data, "#3*", 3) ) { 
				autoMeasure = 1; // Habilita medição automática.
				if(addr >= 16000) {
					// Caso a memória esteja cheia deve-se limpar a memória antes de realizar futuras medições.
					sprintf(out_buffer, "Para fazer mais medicoes limpe a memoria.");
					flag_write = 1;
					autoMeasure = 0; // Desabilita medição automática para o caso de memória cheia.
				}
			}
			else if(str_cmp(Buffer.data, "#4*", 3) ) {
				// Armazena os valores ainda da medição automática não salvos na memória.
				EE241_WriteBlock(addr, &out_buffer_auto[0], cont);
				addr += cont;
				// Desabilita medição automática.
				autoMeasure = 0;
				
			}
			else {
				// Caso o comando de entrada não esteja entre um dos comandos acima.
				sprintf(out_buffer, "%s - Comando Invalido\n\r", Buffer.data);
				flag_write = 1;
			}
			buffer_clean();
			flag_check_command = 0;
		}

		if (autoMeasure && meas){ // Caso tenha dado uma interrupção de tempo, realiza a medição automática dos dados.
			if(addr < 16000){ // Verifica se o ponteiro de endereços está em uma posição válida.
				out_buffer_auto[cont] = value;
				if (cont >=15){
					// Após tomar 16 medições do LDR, armazena os valores obtidos na memória. 
					EE241_WriteBlock(addr, &out_buffer_auto[0], 16);
					cont = 0; // Contador auxiliar para a quantidade de medidas tomadas do LDR.
					addr += 16; // Atualiza o ponteiro de endereço.
				}
				cont++;
				meas = 0; 
			}
			else {
				sprintf(out_buffer, "Memoria Cheia!\r\n");
				flag_write = 1;
				autoMeasure = 0;
			}
		}

		/* Posso construir uma dessas estruturas if(flag) para cada funcionalidade
		 *  do sistema. Nesta a seguir, flag_write e habilitada sempre que alguma outra
		 *  funcionalidade criou uma requisicao por escrever o conteudo do buffer na
		 *  saida UART.
		 */
		if (flag_write == 1) {
			for(i = 0;i < strlen(out_buffer);i++) {
				UART_SendChar(out_buffer[i]);
			}
			buffer_clean();
			flag_write = 0;
		}
	}



	/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
}  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
