/*
 * Servo_Driver.c
 *
 * Created: 2/22/2015 1:30:51 AM
 *  Author: RahulSingh
 */ 
//*****************************************************************************************************//
//						DEFINES						       //
//*****************************************************************************************************//
#define F_CPU 			16000000UL
#define setbit(x,y) 		x |=(1<<y)
#define clearbit(x,y)  		x &=~(1<<y)
#define togglebit(x,y) 		x ^=(1<<y)
#define BAUDRATE		9600

#define ICR_VAL 		1100
#define SERVO_NUMBER		8
#define SERVO_MIN_ANGLE 	0

#define SERVO_MAX_ANGLE 	180
#define ICR1_BASE_VALUE 	1100	//it is base of minimun high time of pulse typically it is 1.1 ms
#define ICR1_TOP_VALUE		3999	//20 ms PULSE i.e. 50Hz frequency
#define FACTOR 			22.22	//It is evaluated as 4000/180 degree

//#define ANGLE_SCALE_FACTOR 	22.22	//It is evaluated as 4000/180 degree => 22.222222

//*****************************************************************************************************//
//					     INCLUDED LIBRARRY		       			       //
//*****************************************************************************************************//
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> 
#include <math.h>


//*****************************************************************************************************//
//	   					GLOBAL VARIBLES					       //
//*****************************************************************************************************//
struct structServo{
	int angle;
	int number;
};

int flag1=1;
int count=0;					//This is the counter for servo timer ISR
struct structServo servoAngleID[8];
struct structServo *servoAngleIDPtr;
char tempData[6]={0,0,0,0,0,0};
char *tempDataPtr;
uint16_t baudRate=103;	//((F_CPU/(16*BAUDRATE))-1);


//*****************************************************************************************************//
//	  					FUNCTION PROTOTYPES	   			       //
//*****************************************************************************************************//
void TimerInit();
void ServoPortInit();
void USARTInit();
void ArrangeAngle(struct structServo servoAngleID[]);

//*****************************************************************************************************//
//	      					    FUNCTIONS  	   				       //
//*****************************************************************************************************//
void TimerInit()
{
	//TCCR1A |=((0<<WGM11)|(0<<WGM10));	 //NO CHANGES IN THE TCCR1A REGISTER
	TCCR1B |=(1<<WGM12)|(1<<WGM13)|(1<<CS11);//CTC MODE(12) - TOP VALUE - ICR1 AND PRESCALAR => 8
	TIMSK |=(1<<TICIE1);			 //ENABLING INTERRUPT FOR TCNT1 REGISTER MATCHING ICR1
}

void ServoPortInit()
{
	DDRB = 0xff;
	PORTB= 0xff;
}

void ServoStructInit()
{
	int i;
	for(i=0; i<8; i++){
		(servoAngleID[i]).number=i;
		(servoAngleID[i]).angle= 20*i;	//Check <int i> maximum value// 16bit unsigned=>256
	}
}

void USARTInit()
{
	//UCSRA=DO NOTHING IN THIS REGISTER
	UCSRB |=(1<<RXCIE)|(1<<RXEN);		//Enabling the Receiver and Receiver Interrupt
	UCSRC |=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
	UBRRL = baudRate;
	UBRRH &=~(1<<URSEL);
	UBRRH = (baudRate<<8);
	//clearbit(UCSRA,RXC);		//RXC bit needs to set to zero before enabling the interrupt
}

void ArrangeAngle(struct structServo servoAngleID[])
{
	//Code for arranging struct <servoAngleID arrary> in arranging order of its servoAngleID[i].angle
}

int main(void)
{
	//servoAnglePtr=&servoAngle;
	ServoPortInit();
	ServoStructInit();
	TimerInit();
	TCNT1=0;
	sei();
    while(1)
    {
	//Call Function ArrangeAngle() 	
    }
}

//*****************************************************************************************************//
//	      					INTERRUPTS  				       	       //
//*****************************************************************************************************//
ISR(TIMER1_CAPT_vect)
{
	TCNT1=0;
	if(count==0){
		PORTB=0xff;
		ICR1=ICR1_BASE_VALUE+(servoAngleID[0].angle)*FACTOR;
	}
	if((1<=count)&& (count<=7)){
		clearbit(PORTB,servoAngleID[count-1].number);
		ICR1=(servoAngleID[count].angle-servoAngleID[count-1].angle)*FACTOR;				//where i is counter and FACTOR is the multiplying factor to scale with ICR1 and angle correspondence
		//Introduce a case for Zero Condition Angle example 2 or more than 2 consecutive angle can have same value. so the difference between the time will be apporximately zero.
	}
	if(count>7){
		clearbit(PORTB,servoAngleID[count-1].number);
		ICR1=ICR1_TOP_VALUE-ICR1_BASE_VALUE-(servoAngleID[7].angle)*FACTOR;
		count=-1;
	}
	count++;
}


ISR(USART_RXC_vect)
{
	while(!(UCSRA &(1<<RXC)));							//For SAFETY Measure
	*tempDataPtr=UDR;
	tempDataPtr++;
	if(*(tempDataPtr-1)==END_OF_SIGNAL){						//STOP CONDITION
		arrangeSignalFlag=1;							//Flag will activate the processing of received data//Arranging
	}
}



















































/*proxymaan123@gmail.com

proxymaan
*/