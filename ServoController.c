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
#define UBRR_VAL		103
#define END_OF_SIGNAL		13						//ENTER KEY

//#define ICR_VAL 		1100
#define SERVO_NUMBER		8
#define SERVO_MIN_ANGLE 	0

#define SERVO_MAX_ANGLE 	180
#define ICR1_BASE_VALUE 	1100						//it is base of minimun high time of pulse typically it is 1.1 ms
#define ICR1_TOP_VALUE		3999						//20 ms PULSE i.e. 50Hz frequency
#define FACTOR 			22.22						//It is evaluated as 4000/180 degree

//*****************************************************************************************************//
//					  INCLUDED LIBRARRY					       //
//*****************************************************************************************************//
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>


//*****************************************************************************************************//
//	   				   GLOBAL VARIBLES					       //
//*****************************************************************************************************//
struct structServo{
	uint16_t angle;
	uint16_t number;
	struct structServo *node;
};

typedef struct structServo servoList;
servoList *tempServoHead;

servoList *servoSwitch;
servoList *servoSwitchDup;
servoList **servoSwitchPtrDup;
servoList **servoSwitchPtr;

servoList **servoSwitchListPtr;
uint16_t servoStartAngle[8]={20,30,40,50,60,70,80,90};

int count=0;									//Counter for servo timer ISR
char tempData[6]={0,0,0,0,0,0};
char *tempDataPtr;
uint16_t baudRate=103;								//=((F_CPU/(16*BAUDRATE))-1);
int arrangeSignalFlag=0;

//Testing Parameters
int testcounter=0;

//*****************************************************************************************************//
//	  				 FUNCTION PROTOTYPES                     		       //
//*****************************************************************************************************//
void USARTInit();
servoList* ArrangeAngle(servoList *servoAngle, servoList *servoNodeHead,int num);
servoList* CreateList();
void PrepareAddToList(char *start);
uint16_t StringToInt(char *dataStart);
void SendInteger(uint16_t num);
servoList* SendList(servoList* head);
void ServoPortInit();
void TimerInit();

//*****************************************************************************************************//
//	      					FUNCTIONS 	   				       //
//*****************************************************************************************************//

servoList* CreateList()
{
	int i;
	servoList *head, *first, *temp = 0;
	first = 0;
	for(i=0; i<8; i++){
		head  = (servoList*)malloc(sizeof(servoList));
		head->angle=servoStartAngle[i];
		head->number=i+1;
		if (first != 0){
			temp->node = head;
			temp = head;
		}
		else
		first = temp = head;
	}
	temp->node = 0;
	temp = first;
	return first;
}

servoList* ArrangeAngle(servoList *servoAngle,servoList *nodeHead,int num)
{
	if(servoAngle->angle<=180 && servoAngle->number<=8){
		servoList *servoNode,*servoPreviousNode;
		servoNode = nodeHead;
		servoPreviousNode=nodeHead;							
		int flagAngle=0;
		int flagNumber=0;

		if(servoAngle->number == nodeHead->number){
//Check the difference between *servoSwitchPtr=servoNodeHead and servoSwitchPtr=&servoNodeHead;
//It does not work with servoSwitchPtr=&nodeHead;
			if(num==1)
				*servoSwitchPtr=nodeHead;
			else
				*servoSwitchPtrDup=nodeHead;
				nodeHead=servoNode->node;
				flagNumber=1;
			}
			if(servoAngle->angle <= nodeHead->angle){
				servoAngle->node=nodeHead;
				nodeHead=servoAngle;
				servoPreviousNode=servoAngle;
				flagAngle=1;
			} 
			while(servoNode !=NULL){
				if((servoAngle->angle <= servoNode->angle)&& flagAngle !=1){
					servoPreviousNode->node=servoAngle;
					servoAngle->node=servoNode;
					servoPreviousNode=servoAngle;
					flagAngle=1;
				}
				if((servoAngle->number==servoNode->number)&& flagNumber!=1){
					if(num==1)
						*servoSwitchPtr=servoNode;
					else
						*servoSwitchPtrDup=servoNode;
					servoPreviousNode->node=servoNode->node;
					flagNumber=1;
				}
				servoPreviousNode=(servoAngle->number==servoNode->number)? servoPreviousNode:servoNode;
				servoNode=servoNode->node;		
			}
			if(flagAngle==0){	
				servoPreviousNode->node=servoAngle;
				servoAngle->node=NULL;
			}
		return nodeHead;
	}
	else{
		return nodeHead;
	}		
}

void PrepareAddToList(char *start)
{
	(*servoSwitchPtr)->number=(*start-48);
	(*servoSwitchPtr)->angle=StringToInt(start+1);
	(*servoSwitchPtrDup)->number=(*start-48);
	(*servoSwitchPtrDup)->angle=StringToInt(start+1);
}

void SendInteger(uint16_t num)
{
	_delay_ms(10);
	while(!(UCSRA && (1<<UDRE)));
	UDR=num;
}

uint16_t StringToInt(char *dataStart)
{
	uint16_t data=0;
	while(*dataStart!=END_OF_SIGNAL){						//ASSIGN THE STOP BIT RECOGNITION
		data=data*10+(*dataStart-48);
		dataStart++;
	}
	return data;
}

servoList* SendList(servoList* head)
{
	servoList *tempServo=head;
	while(tempServo!=NULL){
		while(!(UCSRA && (1<<UDRE)));
		SendInteger(tempServo->angle);
		SendInteger(tempServo->number);
		tempServo=tempServo->node ;
		}
	return head;
}

//ALWAYS KEEP A DELAY OF ABOUT 2MS OR 3MS (MINIMUM) BETWEEN THE TRANSMITTION OF 2 CONSECUTIVE DATA//
int main(void)
{
	USARTInit();
	ServoPortInit();
	TimerInit();

	servoList *servoNodeHead,*servoNodeHeadDup;
	servoNodeHead=CreateList();
	servoNodeHeadDup=CreateList();
	servoSwitchListPtr=&servoNodeHead;
	tempServoHead=*servoSwitchListPtr;
	//servoHead=tempServoHead=servoNodeHead;

	servoSwitch=(servoList*)malloc(sizeof(servoList));
	servoSwitchDup=(servoList*)malloc(sizeof(servoList));
	servoSwitchPtr=&servoSwitch;
	servoSwitchPtrDup=&servoSwitchDup;
	tempDataPtr=tempData;

	clearbit(UCSRA,RXC);
	
SendList(servoNodeHead);
SendList(servoNodeHeadDup);

	TCNT1=0;
	sei();
	while(1){
			//Without Delay the below loop is not executing.
			_delay_us(1); 
//SendInteger(TCNT1);	
//_delay_ms(5000);
//_delay_ms(5000);
			if(arrangeSignalFlag==1){
				PrepareAddToList(tempData);
				//servoNodeHead=ArrangeAngle(*servoSwitchPtr,servoNodeHead,1);
				//	servoSwitchListPtr=&servoNodeHead;
				if(servoSwitchListPtr==&servoNodeHead){
					servoNodeHeadDup=ArrangeAngle(*servoSwitchPtrDup,servoNodeHeadDup,0);
					_delay_ms(50);
					servoSwitchListPtr=&servoNodeHeadDup;
//_delay_ms(5000);
//_delay_ms(5000);
					servoNodeHead=ArrangeAngle(*servoSwitchPtr,servoNodeHead,1);
				}
				else{
					servoNodeHead=ArrangeAngle(*servoSwitchPtr,servoNodeHead,1);
					_delay_ms(50);
					servoSwitchListPtr=&servoNodeHead;
//_delay_ms(5000);
//_delay_ms(5000);
					servoNodeHeadDup=ArrangeAngle(*servoSwitchPtrDup,servoNodeHeadDup,0);
				}
				tempDataPtr=tempData;
				arrangeSignalFlag=0;
				//count=0;
			}
	}
return 0;
}

void USARTInit()
{
	UCSRB |=(1<<RXEN)|(1<<TXEN)|(1<<RXCIE);						//Enabling the Receiver and Receiver Interrupt
	UCSRC |=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
	UBRRL = baudRate;
	UBRRH &=~(1<<URSEL);
	UBRRH = (baudRate<<8);
	clearbit(UCSRA,RXC);								//RXC bit needs to set to zero before enabling interrupt
}

void TimerInit()
{
	//TCCR1A |=((0<<WGM11)|(0<<WGM10));	 					//NO CHANGES IN THE TCCR1A REGISTER
	TCCR1B |=(1<<WGM12)|(1<<WGM13)|(1<<CS11);					//CTC MODE(12) - TOP VALUE - ICR1 AND PRESCALAR => 8
	TIMSK |=(1<<TICIE1);			 					//ENABLING INTERRUPT FOR TCNT1 REGISTER MATCHING ICR1
}

void ServoPortInit()
{
	DDRB = 0xff;
	PORTB= 0xff;
}

//*****************************************************************************************************//
//	      					INTERRUPTS				       	       //
//*****************************************************************************************************//
ISR(TIMER1_CAPT_vect)
{
	//TCNT1=0;
	if(count==0){
		PORTB=0xff;
		ICR1=ICR1_BASE_VALUE+(tempServoHead->angle);				//struct 0 angle
	}
	if((1<=count)&&(count<=7)){
		clearbit(PORTB,	((tempServoHead->number)-1));				//angle [i-1]
		ICR1=(tempServoHead->node->angle-tempServoHead->angle);
		tempServoHead=tempServoHead->node;
			while((tempServoHead->number==tempServoHead->node->number)&& count<=7){
				clearbit(PORTB,((tempServoHead->number)-1));
				tempServoHead=tempServoHead->node;
				count++;
			}
	}
	if(count>7){
		clearbit(PORTB,((tempServoHead->number)-1));
		ICR1=ICR1_TOP_VALUE-ICR1_BASE_VALUE-(tempServoHead->angle);
		tempServoHead=*servoSwitchListPtr;
		count=-1;
//		testcounter++;
//		if(testcounter>200){
//			SendList(*servoSwitchListPtr);
//			testcounter=0;
//		}
	}
	count++;
	TCNT1=0;
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
