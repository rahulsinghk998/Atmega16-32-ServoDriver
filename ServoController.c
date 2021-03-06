/*
 * Servo_Driver.c
 *
 * Created: 2/22/2015 1:30:51 AM
 *  Author: RahulSingh
 */ 
//*****************************************************************************************************//
//						DEFINES						       //
//*****************************************************************************************************//
#define F_CPU 				16000000UL
#define setbit(x,y) 		x |=(1<<y)
#define clearbit(x,y)  		x &=~(1<<y)
#define togglebit(x,y) 		x ^=(1<<y)
#define BAUDRATE			9600
#define UBRR_VAL			103
#define END_OF_SIGNAL		13		//ENTER KEY

#define ICR_VAL 			1100
#define SERVO_NUMBER		8
#define SERVO_MIN_ANGLE 	0

#define SERVO_MAX_ANGLE 	180
#define ICR1_BASE_VALUE 	1100	//it is base of minimun high time of pulse typically it is 1.1 ms
#define ICR1_TOP_VALUE		3999	//20 ms PULSE i.e. 50Hz frequency
#define FACTOR 				22.22	//It is evaluated as 4000/180 degree

//#define ANGLE_SCALE_FACTOR 	22.22	//It is evaluated as 4000/180 degree => 22.222222

//*****************************************************************************************************//
//										 INCLUDED LIBRARRY		       							       //
//*****************************************************************************************************//
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>


//*****************************************************************************************************//
//	   										GLOBAL VARIBLES										       //
//*****************************************************************************************************//
struct structServo{
	uint16_t angle;
	uint16_t number;
	struct structServo *node;
};

typedef struct structServo servoList;
servoList *servoHead;					//To store the head pointer
servoList *tempServoHead;

servoList *servoSwitch;
servoList **servoSwitchPtr;

servoList *servoSwitchHead;
servoList **servoSwitchHeadPtr;

uint16_t servoStartAngle[8]={25,50,75,100,125,150,160,179};

int flag1=1;
int count=0;									//Counter for servo timer ISR

char tempData[6]={0,0,0,0,0,0};
char *tempDataPtr;
uint16_t baudRate=103;								//=((F_CPU/(16*BAUDRATE))-1);
int arrangeSignalFlag=0;

//*****************************************************************************************************//
//	  									  FUNCTION PROTOTYPES	   									   //
//*****************************************************************************************************//
void USARTInit();
servoList* ArrangeAngle(servoList *servoAngle, servoList *servoNodeHead);
servoList* CreateList();
void PrepareAddToList(char *start);
uint16_t StringToInt(char *dataStart);
void SendInteger(uint16_t num);
servoList* SendList(servoList* head);
void ServoPortInit();
void TimerInit();

//*****************************************************************************************************//
//	      									  FUNCTIONS					 	   				       //
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

servoList* ArrangeAngle(servoList *servoAngle,servoList *nodeHead)
{
	if(servoAngle->angle<=180 && servoAngle->number<=8){
		servoList *servoNode;
		servoList *servoPreviousNode;
		servoNode = nodeHead;
		servoPreviousNode=nodeHead;						//CHANGED
		int flagAngle=0;
		int flagNumber=0;
		if(servoAngle->number == nodeHead->number){
//Check the difference between *servoSwitchPtr=servoNodeHead and servoSwitchPtr=&servoNodeHead;
			*servoSwitchPtr=nodeHead;
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
				*servoSwitchPtr=servoNode;
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
	while(*dataStart!=END_OF_SIGNAL){	//ASSIGN THE STOP BIT RECOGNITION
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
	tempDataPtr=tempData;
	servoList *servoNodeHead_a,*servoNodeHead_b;

	servoList *tempServoSwitch;

	servoNodeHead_a=CreateList();
	servoNodeHead_b=CreateList();
	servoSwitchHeadPtr=&servoNodeHead_a;

	servoHead=tempServoHead=servoNodeHead;
	servoSwitch=(servoList*)malloc(sizeof(servoList));
	servoSwitchPtr=&servoSwitch;			//Try to eliminate the effect of **pointer usage

	clearbit(UCSRA,RXC);
	USARTInit();
	ServoPortInit();
	TimerInit();
	TCNT1=0;
	sei();
	while(1){
			//Without Delay the below loop is not executing
			if(arrangeSignalFlag==1){
			PrepareAddToList(tempData);

			tempServoSwitch=*servoSwitchPtr;

			//servoNodeHead=ArrangeAngle(*servoSwitchPtr,servoNodeHead);
			if(servoSwitchHeadPtr==&servoNodeHead_a){
				servoNodeHead_b=ArrangeAngle(*servoSwitchPtr,servoNodeHead_b);
				servoSwitchHeadPtr=&servoNodeHead_b;
				servoNodeHead_a=ArrangeAngle(tempServoSwitch,servoNodeHead_a);
			}
			else{
				servoNodeHead_a=ArrangeAngle(*servoSwitchPtr,servoNodeHead_a);
				servoSwitchHeadPtr=&servoNodeHead_a;
				servoNodeHead_b=ArrangeAngle(tempServoSwitch,servoNodeHead_b);
			}	
			servoHead=servoNodeHead;	//
			//SendList(servoNodeHead);
			tempDataPtr=tempData;
			arrangeSignalFlag=0;
			count=0;
			}
	}
return 0;
}

void USARTInit()
{
	UCSRB |=(1<<RXEN)|(1<<TXEN)|(1<<RXCIE);					//Enabling the Receiver and Receiver Interrupt
	UCSRC |=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
	UBRRL = baudRate;
	UBRRH &=~(1<<URSEL);
	UBRRH = (baudRate<<8);
	//clearbit(UCSRA,RXC);							//RXC bit needs to set to zero before enabling interrupt
}

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

//*****************************************************************************************************//
//	      									INTERRUPTS  								       	       //
//*****************************************************************************************************//
ISR(TIMER1_CAPT_vect)
{
	if(count==0){
		PORTB=0xff;
		ICR1=ICR1_BASE_VALUE+(tempServoHead->angle)*FACTOR;			//struct 0 angle
	}
	while((tempServoHead->number==tempServoHead->node->number) && count <=7){
		clearbit(PORTB,tempServoHead->number);
		tempServoHead=tempServoHead->node;
		count++;	
	}
	if((1<=count)&&(count<=7)){
		clearbit(PORTB,tempServoHead->number);					//angle [i-1]
		ICR1=(tempServoHead->node->angle-tempServoHead->angle)*FACTOR;
		tempServoHead=tempServoHead->node;
	}
	if(count>7){
		clearbit(PORTB,tempServoHead->number);
		ICR1=ICR1_TOP_VALUE-ICR1_BASE_VALUE-(tempServoHead->angle)*FACTOR;
		tempServoHead=*servoSwitchHeadPtr;
		count=-1;
	}
	count++;
	TCNT1=0;
}

ISR(USART_RXC_vect)
{
	while(!(UCSRA &(1<<RXC)));	//For SAFETY Measure
	*tempDataPtr=UDR;
	tempDataPtr++;
	if(*(tempDataPtr-1)==END_OF_SIGNAL){	//STOP CONDITION
		arrangeSignalFlag=1;	//Flag will activate the processing of received data//Arranging
	}
}
