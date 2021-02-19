/*
 * main implementation: use this 'C' sample to create your own application
 *
 */

#include "derivative.h" /* include peripheral declarations */

//Variables de Estados
unsigned char FC;      		                                 //Contador de Frecuencia Cardiaca
unsigned char RC; 	  		                                //Ritmo Cardiaco
unsigned long trash; 		                               //Variable para calcular Ritmo Basal
unsigned long trash2;                                     //Varibale para calcular Ritmo Basal
unsigned long trash4;                       			 //Acumulador de Ataques
unsigned long trash3;     		               			//Acumulador de Ataques
unsigned char FC_Basal; 	       	                   //Ritmo Basal Cardiaco
unsigned char FR_Basal;                               //Ritmo Basal Respiratorio
unsigned char C=0;      	                         //Control de Calibriacion
unsigned char n=0;      		                    //Contador para repetir ciclo de Calibracion
unsigned char dato1;    		                   //Trash de Uart
unsigned char grabacion=0; 		                  //Triger de Ataque
unsigned short lFR=1188;                         //Largo de Datos
unsigned long i=0;                              //Contador de FR
unsigned long FR_SF[1200];                     //Arreglo Frecuencia Respiratoria sin Filtrar
unsigned long FR_MM[1188];                    //Arreglo de Frecuencia Respiratoria ya Filtrado
unsigned char FR;           	             //Frecuencia Respiratoria
unsigned long FR_ataque[20];  	            //Frecuencia Respiratoria de Ataque
unsigned long FC_ataque[20];               //Frecuencia Cardiaca de Ataque
unsigned long Duracion[20];               //Duracion de Ataque
unsigned long na=0;                      //Numero de Ataques
unsigned char at=1;                     //Controlador de Ataque
char mens1_ASCII[]={"FC,FR,EDA,t"};    //Arreglo de Mensaje de ASCII
char mens_ASCII[]={"   ,   ,   ,   "};//Mensaje a Mendar
unsigned long uFR=0.1;

void vGPIO_init(void)
{
	SIM_SCGC5|=(1<<10)+1; //Clock PortB
	PORTB_PCR3=(1<<8)+(9<<16); //PTB3 as GPIO
	NVICISER1|=1<<(60%32); //Interrupción de Puerto B
}
void vUART_init(void)
{
	//UART4
	//PTC15
	SIM_SCGC1|=(1<<10);//UART 4
	SIM_SCGC5|=(1<<11);//PORT C
	
	PORTC_PCR15|=(3<<8); //UART Tx
	PORTC_PCR14|=(3<<8); //UART Rx
	
	UART4_BDL|=137; //Baud Rate 9600
	
	UART4_C2|=12+(1<<5); // RE y RIE(interrupcion) enable
	
	NVICISER2|=(1<<(66%32));

}
void initADC_EDA(void){
	SIM_SCGC6|=(1<<27);
	NVICISER1|=(1<<(39%32));//Interrupción ADC0
	//habilitar nvic no habilitar interrupción
}
void initADC_FR(void){
	SIM_SCGC3|=(1<<27);
	NVICISER2|=(1<<(73%32));//Interrupción ADC1
	//habilitar nvic no habilitar interrupción
}
void vLPTMR0_init(void){
	LPTMR0_PSR=5;            //Bypass, LPO (1 KHz)
	LPTMR0_CSR=0x41;        //Hab Intr, Timer Enable
	LPTMR0_CMR=60000-1;    // 1 minuto
	NVICISER1=1<<(58%32); //Vector 58
}
void init_LPIT_EDA(void){
	//PIT0
	SIM_SCGC6|=(1<<23);
	PIT_MCR&=~(1<<1);
	PIT_LDVAL0=63550; //(640*2a la 15)/330hz
	PIT_TCTRL0=3; //Interrupt enable y timer enable
	NVICISER1|=(1<<(48%32));
}
void init_LPIT_FR(void){
	//PIT1
	SIM_SCGC6|=(1<<23);
	PIT_MCR&=~(1<<1);
	PIT_LDVAL1=1048576; //(640*2a la 15)/20hz
	PIT_TCTRL1=3; //Interrupt enable y timer enable
	NVICISER1|=(1<<(49%32));
}


int main(void)
{
	
	vGPIO_init();
	vUART_init();
	initADC_EDA();
	initADC_FR();
	vLPTMR0_init();
	init_LPIT_EDA();
	init_LPIT_FR();
	
	return 0;
}


void vMedianMovil(unsigned long X[],unsigned long Y[],unsigned short N,unsigned short L){
	short i,c;
	double sum;
	for(c=0;L-N>c;c++){
		sum=0;
		for(i=0;N>i;i++){
			sum=sum+X[c+i];
		}
		Y[c]=sum/N;
	}
}
short sMax(unsigned long X[],unsigned long class, unsigned short L){
	unsigned short ctr=0;
	unsigned short c=0;
	unsigned short sum=0;
	for(c=2;L>c;c++){
	 if(X[i]>X[i-1]){
		 if(X[i]>class){
			ctr=1; 
		 }
	 }
	 if(X[i]<X[i-1]){
		 if(X[i]<class){
		    if(ctr==1){
		      sum++;
		      ctr=0;
		    } 
		 } 
	 }
	}return sum;
}
void vUART_send(unsigned char dato)
{
		do {} while (!(UART4_S1 & 0x80));
		UART4_D=dato; 
}
void vUART_sendmessage (char message[]){
	unsigned char i=0;
	do{
		vUART_send(message[i++]);
	}while(message[i] != 0);
}
void UART4_Status_IRQHandler(void)
{
  (void)UART4_S1; //Apaga bandera]
  dato1=UART4_D;
  if(dato1=='C'){
	  C=1;
  	  }
  if(dato1=='R'){
	  unsigned long n=0;
	  vUART_sendmessage(mens1_ASCII);
	  do{
		  //FC
		  mens_ASCII[2]=(FC_ataque[n]%10)+'0';
		  FC_ataque[n]/=10;
		  mens_ASCII[1]=(FC_ataque[n]%10)+'0';
		  FC_ataque[n]/=10;
		  mens_ASCII[0]=(FC_ataque[n]%10)+'0';
		  FC_ataque[n]/=10;
		  
		  //FR
		  mens_ASCII[6]=(FC_ataque[n]%10)+'0';
		  FR_ataque[n]/=10;
		  mens_ASCII[5]=(FC_ataque[n]%10)+'0';
		  FR_ataque[n]/=10;
		  mens_ASCII[4]=(FC_ataque[n]%10)+'0';
		  FR_ataque[n]/=10;
		  
		  //EDA
		  
		  //Tiempo
		  
		  mens_ASCII[14]=(Duracion[n]%10)+'0';
		  Duracion[n]/=10;
		  mens_ASCII[13]=(Duracion[n]%10)+'0';
		  Duracion[n]/=10;
		  mens_ASCII[11]=(Duracion[n]%10)+'0';
		  Duracion[n]/=10;
		  
		  vUART_sendmessage(mens_ASCII);
		  
		  n++;
	  }while(n<na);
  	  }
}
void PORTB_IRQHandler(void)
{
	PORTB_PCR3|=(1<<24); //Desactivar Bandera de PortB
    FC++;			     //Aumentar Contador
}
void LPTimer_IRQHandler (void)
{
		LPTMR0_CSR|=0x80;          //Apagar bandera
		if(C==1){             //Estado de Calibracion
		  if(n<5){
			  vMedianMovil(FR_SF,FR_MM,18,lFR);
			  FR=sMax(FR_MM,uFR,lFR);
			  trash2=trash2+FR;
			  trash=trash+FC;   //Acumulador de Cuenta
			  FC=0;			   //Reiniciar Cuenta
			  FR=0;            //Reiniciar Cuenta
			  n++;			  //Acumulador de Repeticion
		  }else{
			  FC_Basal=trash/5;   //Asignacion de Frecuencia Cardiaca Basal
			  FR_Basal=trash2/5; //Asignacion de Frecuencia Respiratoria Basal
			  FC=0;
			  FR=0;
			  C=0;}           //Reiniciar Cuenta
		}else{
			RC=FC;        	 //Lectura de Medicion cada 1 minuto
			vMedianMovil(FR_SF,FR_MM,12,lFR);
			FR=sMax(FR_MM,uFR,lFR);
			FC=0;		    //Reiniciar Variable
			if((FR>(FR_Basal*1.30)) && (RC>(FC_Basal*1.10)) ){
				trash3=trash3+FR;
				trash4=trash4+RC;
				n++;
				at=1;
			}
			if(at==1){
			    FR_ataque[na]=trash3/n;
			    FC_ataque[na]=trash4/n;
			    Duracion[na]=n;
			    if((FR<=(FR_Basal*1.30)) && (RC<=(FC_Basal*1.10)) ){
			    	n=0;
			        at=0;
			        na++;
			    }
			}
			
		}
}
void PIT0_IRQHandler(){
	//EDA
	PIT_TFLG0=1;
	ADC0_SC1A|=1;//ENABLE interruption of adc(bit6), modo diferencial (bit5) y AD1 as input
	}
void PIT1_IRQHandler(){
	//FR
	ADC1_SC1A =(1<<6)+18;//ENABLE interruption of adc(bit6), modo diferencial (bit5) y AD1 as input
	PIT_TFLG1=1;
}
void ADC0_IRQHandler(){
	
}
void ADC1_IRQHandler(){
	if(i>1200){
    i=0;}
	ADC1_SC1A = (1 << 7) + 0x1F;
	FR_SF[i++]=ADC1_RA;
}

