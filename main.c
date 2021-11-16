#include <p24fxxxx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <p24fj256gb110.h>

#define TEMPERATURE 2 // Sensor de temperatura ligado a AN2


// Configuration Bits
#ifdef __PIC24FJ64GA004__ //Defined by MPLAB when using 24FJ64GA004 talk_to_slave
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx1 & IOL1WAY_ON) 
_CONFIG2( FCKSM_CSDCMD & OSCIOFNC_OFF & POSCMOD_HS & FNOSC_PRI & I2C1SEL_SEC)
#else
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx2) 
_CONFIG2( FCKSM_CSDCMD & OSCIOFNC_OFF & POSCMOD_HS & FNOSC_PRI)
#endif

/*******************************************/
/*Nome: inicializar_ADC ********************/
/*Função: Inicializa ADC *******************/
/*******************************************/
void inicializar_ADC ()
{
	AD1PCFG = 0xffd1; // select analog input pins 
				//0xffd1 -> 1111 1111 1101 0001
	AD1CON1 = 0x8000; // automatic conversion start after sampling;; o 8 e para UART com 8 bits
	AD1CSSL = 0; // no scanning required
} 

/*******************************************/
/*Nome: inicializar_UART *******************/
/*Função: Inicializa o protocolo UART ******/
/*******************************************/
void inicializar_UART(){
	U2BRG = 25; //Valor calculado atraves da formula presente no datasheet, para um baud rate de 9600
	U2STA = 0;
	U2MODE = 0x8000; //Enable Uart for 8-bit data
	//no parity, 1 STOP bit
	U2STAbits.UTXEN = 1; //Enable Transmit
}

/*******************************************/
/*Nome: inicializar_I2C ********************/
/*Função: Inicializar o protocolo I2C ******/
/*******************************************/
void inicializar_I2C () {
	I2C2CON = 0x8000; //Controlar o registo I2C2CO
	I2C2BRG = 39; //BAUDRATE = 4MHZ
}

/*******************************************/
/*Nome: atraso *****************************/
/*Função: Cria um atraso********************/
/*******************************************/
void atraso(long delay){

	long timer = 0;
	
	for (timer; timer < delay; timer++); //delay
}

/*******************************************/
/*Nome: putChar_UART ***********************/
/*Função: Por um caracter na UART **********/
/*******************************************/
void putChar_UART(char c){
	U2TXREG = c;
	atraso(2000);
}

/********************************************/
/*Nome: getCharUART *************************/
/*Função: Tirar um caracter na UART *********/
/********************************************/
char getChar_UART(){
	
	while (!U2STAbits.URXDA); //verifica se ha chars para ler
	IFS1bits.U2RXIF = 0; // flag de interrupt de transmissão igual a zero
		
	return U2RXREG;
}

/******************************************/
/*Nome: putStringUART *********************/
/*Função: Por uma string na UART **********/
/******************************************/
void putString_UART(char *string){    //*string -> apontador
	
	int i = 0; 
	int length = strlen(string);  //strlen = tamanho da string

	for (i = 0; i < length; i++){
		putChar_UART(*string);   // envia o caracter 
		*string++;// avança no apontador da string
	}
}

/****************************************************/
/*Nome: startI2C ************************************/
/*Função: Inicia a comunicação I2C ******************/
/****************************************************/
void startI2C(void) {
	//Inicia os pinos SDA e SCL
	I2C2CONbits.SEN = 1;
	while(I2C2CONbits.SEN);
}

/********************************************************/
/*Nome: stopI2C *****************************************/
/*Função: Termina a comunicação I2C *********************/
/********************************************************/
void stopI2C(void) {
	//Ativa o bit PEN do registo I2C2CON para terminar a transmissão
	I2C2CONbits.PEN = 1;
	while(I2C2CONbits.PEN);
}

/******************************************/
/*Nome: sensor_temperatura ****************/
/*Função: Retorna o valor da temperatura **/
/******************************************/
int sensor_temperatura() {
	AD1CHS = TEMPERATURE; // Sensor de temperatura ligado a AN2
	AD1CON1bits.SAMP = 1; // start sampling
	atraso(2000);
	AD1CON1bits.SAMP = 0; // end sampling
	while (!AD1CON1bits.DONE); // wait for conversion to br completed	
	return ADC1BUF0; // get ADC value  (1-1024)	
}

/*Nome: write_data_I2C ********************/
/*Função: Envia dados para o I2C **********/
/******************************************/
void write_data_I2C(unsigned char dados) {

	char data[30];

	sprintf(data, " Sent: %X \r\n", dados);

	I2C2TRN = dados;

	putString_UART(data);


	if (I2C2STATbits.IWCOL) {
		putString_UART("Erro. \r\n");
		//return 0; // Error, collision writing
	}

	while (I2C2STATbits.TBF); //Se o bit TBF do registo I2C2STAT estiver ativo o buffer está ocupado
	while (I2C2STATbits.TRSTAT);
}

/**********************************************************/
/*Nome: request_I2C ***************************************/
/*Função: Recebe uma mensagem do I2C **********************/
/**********************************************************/
int request_I2C(int bits) {

	putString_UART("Receiving... \r\n");

	I2C2CONbits.RCEN = 1;  //Ativa o modo receive do I2C

	while (I2C2CONbits.RCEN); //

	while (!I2C2STATbits.RBF); //

	unsigned char response = I2C2RCV;

	int resposta = (int)response;
	//I2C2CONbits.ACKEN = 1;
	//putString_UART("response \r\n");
	if (bits == 2) { //Recebe uma mensagem de 2 byte(16 bits)
		I2C2CONbits.ACKDT = 0;
		I2C2CONbits.ACKEN = 1;
		while (I2C2CONbits.ACKEN);

		I2C2CONbits.RCEN = 1;

		while (I2C2CONbits.RCEN); 

		while (!I2C2STATbits.RBF); // Quando a 1 significa que o registo RBF está cheio logo o receive está completo

		resposta = ((I2C2RCV << 8) + response);

		I2C2CONbits.ACKDT = 1;
		I2C2CONbits.ACKEN = 1;

		while (I2C2CONbits.ACKEN);
	}

	return resposta;  //Retorna a mensagem proveniente do I2C
}

/**********************************************************/
/*Nome: Talk_to_slave**************************************/
/*Função: Inicializa uma conversa com um slave(envia e recebe dados****/
/**********************************************************/
int talk_to_slave(unsigned char addr_slave, unsigned char cmd, int stat_receive, int n_bits) {

	unsigned char addrs;
	int rec;
	char string[30];
	startI2C();


	addrs = ((addr_slave << 1) | 0);  //shift e mete o primeiro bit a 0 para enviar
	write_data_I2C(addrs);
	write_data_I2C(cmd);


	stopI2C();

	if (stat_receive) { // se tiver alguma resposta do slave
		startI2C();

		addrs = ((addr_slave << 1) | 1); //shift e mete o primeiro bit a 1 para receber

		write_data_I2C(addrs);

		rec = request_I2C(n_bits);

		sprintf(string, " Received: %d \r\n", rec);
		putString_UART(string);

		stopI2C();

		return rec;
	}

	return 0;// Error
}

/*****************************************************/
/*Nome: Motor1 ***************************************/
/*Função: Consoante a variável de entrada, 1 ou 0, liga ou desliga o motor 1 ******/
/*****************************************************/
void Motor1(int estado) { 
	putString_UART("Motor 1 ativo!\r\n");
	if (estado == 1)	talk_to_slave(0x0C, 0x51, 0, 1);  //liga
	else talk_to_slave(0x0C, 0x53, 0, 1);  //desliga
}

/*****************************************************/
/*Nome: Motor2 ***************************************/
/*Função: Consoante a variável de entrada, 1 ou 0, liga ou desliga o motor 2 ******/
/*****************************************************/
void Motor2(int estado) { 
	putString_UART("Motor 2 ativo!\r\n");
	if (estado == 1)	talk_to_slave(0x0C, 0x52, 0, 1); //liga
	else talk_to_slave(0x0C, 0x54, 0, 1);  //desliga
}

/*****************************************************/
/*Nome: Motor3 ***************************************/
/*Função: Consoante a variável de entrada, 1 ou 0, liga ou desliga o motor 3 ******/
/*****************************************************/
void Motor3(int estado) { 
	putString_UART("Motor 3 ativo!\r\n");
	if (estado == 1)	talk_to_slave(0x0D, 0x61, 0, 1); //liga
	else talk_to_slave(0x0D, 0x62, 0, 1); //desliga
}

/***************************************************************************************************/
/*Nome: LEDS ****************************************************************************************/
/*Função: Consoante a variável de entrada, G, Y, R ou N liga o LED verde, amarelo, vermelho ou desliga os leds ********/
/***************************************************************************************************/
void LEDS(char cor) {
	putString_UART("Actua LEDs.\r\n");
	if(cor == 'G' || cor == 'g'){ //Liga leds verdes
		putString_UART("Green light.\r\n");
		talk_to_slave(0x1A, 0x81, 0, 1);
	}
	else if (cor == 'R' || cor == 'r') { //Liga leds vermelhos
		putString_UART("Red light.\r\n");
		talk_to_slave(0x1A, 0x82, 0, 1);
	}
	else if (cor == 'Y' || cor == 'y') { //Liga leds amarelos
		putString_UART("Yellow light.\r\n");
		talk_to_slave(0x1A, 0x83, 0, 1);
	}
	else if (cor == 'N' || cor == 'n') { //Desliga leds
		putString_UART("Turn of lights.\r\n");
		talk_to_slave(0x1A, 0x84, 0, 1);
	}
	else if (cor == 'E' || cor == 'e') { //Error
		putString_UART("Lights Error.\r\n");
		talk_to_slave(0x1A, 0x00, 0, 1);
	}
}

/**********************************************************************************/
/*Nome: buzzer ********************************************************************/
/*Função: Consoante a variável de entrada, 1 ou 0, liga ou desliga o buzzer *******/
/**********************************************************************************/
void buzzer(int estado) { //Activa a sirene de alarme
	putString_UART("DANGER! Get to the choppa!\r\n");
	if (estado == 1)	talk_to_slave(0x1B, 0x71, 0, 1);
	else if (estado == 2) talk_to_slave(0x1B, 0x72, 0, 1);
	else talk_to_slave(0x1B, 0x00, 0, 1);
}

/**********************************************************/
/*Nome: Emergency_Stop*************************************/
/*Função: Para todos os motores e mete led amarelo ********/
/**********************************************************/
void Emergency_Stop() {
	Motor1(0);
	Motor2(0);
	Motor3(0);
	LEDS('Y');
	putString_UART("Emergency Stop! All motors deactivated.\r\n");
}

/**********************************************************/
/*Nome: Fire_Alarm ****************************************/
/*Função: Para todos os motores e mete led vermelho *******/
/**********************************************************/
void Fire_Alarm() {
	Motor1(0);
	Motor2(0);
	Motor3(0);
	LEDS('R');
	putString_UART("Fire detected! Get to the choppa!\r\n");
}

/******************************************************************/
/*Nome: hall_sensor ***********************************************/
/*Função: Retorna o valor lido pelo sensor de hall ****************/
/******************************************************************/
int sensor_Hall() { 
	int treshold = 500;
	putString_UART("Hall sensor...\r\n");
	if(talk_to_slave(0x0E, 0x30, 1, 2) < treshold)	return 1; //peça detectada
	else if (talk_to_slave(0x0E, 0x30, 1, 2) > treshold) return 0; //peça n detectada
	else return 99;//error
}

/******************************************************************/
/*Nome: sensor_UltraSom *****************************************/
/*Função: Retorna o valor lido pelo sensor de ultrasom ****************/
/******************************************************************/
int sensor_UltraSom() {
	putString_UART("Sensor Ultrasom...\r\n");
	if (talk_to_slave(0x0A, 0x10, 1, 1) == 26) return 1; //peça detectada
	else if (talk_to_slave(0x0A, 0x10, 1, 1) == 27) return 0; //peça não detectada
	else return 99; //error
}

/******************************************************************/
/*Nome: sensor_indutivo *****************************************/
/*Função: Retorna o valor lido pelo sensor indutivo****************/
/******************************************************************/
int sensor_indutivo() {
	putString_UART("Sensor Indutivo...\r\n");
	if (talk_to_slave(0x0F, 0x20, 1, 1) == 42) return 1; //porta fechada
	else if (talk_to_slave(0x0F, 0x20, 1, 1) == 43) return 0; //porta aberta
	else return 99; //error
}

/******************************************************************/
/*Nome: fimdocurso*****************************************/
/*Função: Retorna o valor lido pelo sensor indutivo****************/
/******************************************************************/
int fimdocurso() {
	putString_UART("Checking course...\r\n");
	if (talk_to_slave(0x0B, 0x40, 1, 1) == 65) return 1; //peça detectada
	else if (talk_to_slave(0x0B, 0x40, 1, 1) == 66) return 0; //peça não detectada
	else return 99; //error
}

/**********************************************************/

int main(void)
{

	inicializar_ADC(); //inicialização do ADC
	inicializar_UART(); //inicialização da UART
	inicializar_I2C(); //inicialização do I2C
	LEDS('G');
	buzzer(2);
	
	int peca_1 = 0;
	int peca_2 = 0;
	int pecaCurso=0;
	int total_peca = 0;
	
	int Room_temp = 0;
	int temperature_threshold = 55; 	


	
	while ( 1 ){

		putString_UART("Begin...\r\n");
		//Leitura dos sensores
		Room_temp = sensor_temperatura();
		total_peca = peca_1 + peca_2;

		
		if (Room_temp/10 >= temperature_threshold) { //FIRE
			Fire_Alarm();
			buzzer(1);
			while (Room_temp / 10 >= temperature_threshold);
			LEDS('G');
			buzzer(0);
		}
		else if(!sensor_indutivo()){ 
			Emergency_Stop();
			while (!sensor_indutivo());
			LEDS('G');
		}
		else if (sensor_UltraSom() && pecaCurso ==0) { //Peça detetada no sensor de Ultra Som
			Motor1(1);
			putString_UART("Motor 1 ativo.\r\n");
			peca_1 = peca_1 + 1;
			pecaCurso = 1;
		}
		else if (sensor_Hall() && pecaCurso ==0) { //Peça detetada no sensor de Hall
			Motor2(1);
			putString_UART("Motor 2 ativo.\r\n");
			peca_2 = peca_2 + 1;
			pecaCurso = 1;
		}
		else if(fimdocurso() && pecaCurso==1){
			//Motores laterais desativados
			Motor1(0);
			putString_UART("Motor 1 desativado\r\n");
			Motor2(0);
			putString_UART("Motor 2 desativado\r\n");
			//Motor armazem ativo
			Motor3(1);
			putString_UART("Motor 3 ativo\r\n");
			atraso(300000);
			Motor3(0);
			putString_UART("Motor 3 desativado\r\n");
			pecaCurso = 0;
		}
	putString_UART("END...\r\n");
	}
}

