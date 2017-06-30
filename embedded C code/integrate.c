
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include <math.h> //included to support power function
#include "lcd.h"

unsigned long int ShaftCountLeft = 0; //to keep track of left position encoder 
unsigned long int ShaftCountRight = 0; //to keep track of right position encoder
unsigned int Degrees; //to accept angle in degrees for turning
int distance=0;
void port_init();
void timer5_init();
void velocity(unsigned char, unsigned char);
void motors_delay();

void lcd_port_config (void)
{
 DDRC = DDRC | 0xF7; //all the LCD pin's direction set as output
 PORTC = PORTC & 0x80; // all the LCD pins are set to logic 0 except PORTC 7
}
char color='n'; //n=no
unsigned char ADC_Conversion(unsigned char);
unsigned char ADC_Value;
unsigned char flag = 0;
unsigned char flag1 = 0;
unsigned char flag2 = 0;
unsigned char Left_white_line = 0;
unsigned char Center_white_line = 0;
unsigned char Right_white_line = 0;
unsigned char Front_Sharp_Sensor=0;
unsigned char Right_Sharp_Sensor=0;
unsigned char Left_IR_Sensor=0;
volatile unsigned long int pulse = 0; //to keep the track of the number of pulses generated by the color sensor
volatile unsigned long int red;       // variable to store the pulse count when read_red function is called
volatile unsigned long int blue;      // variable to store the pulse count when read_blue function is called
volatile unsigned long int green;     // variable to store the pulse count when read_green function is called

//ADC pin configuration
void adc_pin_config (void)
{
 DDRF = 0x00; 
 PORTF = 0x00;
 DDRK = 0x00;
 PORTK = 0x00;
}

void servo1_pin_config (void)
{
 DDRB  = DDRB | 0x20;  //making PORTB 5 pin output
 PORTB = PORTB | 0x20; //setting PORTB 5 pin to logic 1
}


unsigned char data; //to store received data from UDR1

void buzzer_pin_config (void)
{
 DDRC = DDRC | 0x08;		//Setting PORTC 3 as outpt
 PORTC = PORTC & 0xF7;		//Setting PORTC 3 logic low to turnoff buzzer
}

void motion_pin_config (void)
{
 DDRA = DDRA | 0x0F;
 PORTA = PORTA & 0xF0;
 DDRL = DDRL | 0x18;   //Setting PL3 and PL4 pins as output for PWM generation
 PORTL = PORTL | 0x18; //PL3 and PL4 pins are for velocity control using PWM.
}

void color_sensor_pin_config(void)
{
	DDRD  = DDRD | 0xFE; //set PD0 as input for color sensor output
	PORTD = PORTD | 0x01;//Enable internal pull-up for PORTD 0 pin
}



void left_position_encoder_interrupt_init (void) //Interrupt 4 enable
{
 cli(); //Clears the global interrupt
 EICRB = EICRB | 0x02; // INT4 is set to trigger with falling edge
 EIMSK = EIMSK | 0x10; // Enable Interrupt INT4 for left position encoder
 sei();   // Enables the global interrupt 
}

void right_position_encoder_interrupt_init (void) //Interrupt 5 enable
{
 cli(); //Clears the global interrupt
 EICRB = EICRB | 0x08; // INT5 is set to trigger with falling edge
 EIMSK = EIMSK | 0x20; // Enable Interrupt INT5 for right position encoder
 sei();   // Enables the global interrupt 
}

//ISR for right position encoder
ISR(INT5_vect)  
{
 ShaftCountRight++;  //increment right shaft position count
}


//ISR for left position encoder
ISR(INT4_vect)
{
 ShaftCountLeft++;  //increment left shaft position count
}

void color_sensor_pin_interrupt_init(void) //Interrupt 0 enable
{
	cli(); //Clears the global interrupt
	EICRA = EICRA | 0x02; // INT0 is set to trigger with falling edge
	EIMSK = EIMSK | 0x01; // Enable Interrupt INT0 for color sensor
	sei(); // Enables the global interrupt
}

//ISR for color sensor
ISR(INT0_vect)
{
	pulse++; //increment on receiving pulse from the color sensor
}

void buzzer_on (void)
{
 unsigned char port_restore = 0;
 port_restore = PINC;
 port_restore = port_restore | 0x08;
 PORTC = port_restore;
}

void buzzer_off (void)
{
 unsigned char port_restore = 0;
 port_restore = PINC;
 port_restore = port_restore & 0xF7;
 PORTC = port_restore;
}


//Function to configure INT4 (PORTE 4) pin as input for the left position encoder
void left_encoder_pin_config (void)
{
 DDRE  = DDRE & 0xEF;  //Set the direction of the PORTE 4 pin as input
 PORTE = PORTE | 0x10; //Enable internal pull-up for PORTE 4 pin
}

//Function to configure INT5 (PORTE 5) pin as input for the right position encoder
void right_encoder_pin_config (void)
{
 DDRE  = DDRE & 0xDF;  //Set the direction of the PORTE 4 pin as input
 PORTE = PORTE | 0x20; //Enable internal pull-up for PORTE 4 pin
}



// Timer 5 initialized in PWM mode for velocity control
// Prescale:256
// PWM 8bit fast, TOP=0x00FF
// Timer Frequency:225.000Hz
void timer5_init()
{
	TCCR5B = 0x00;	//Stop
	TCNT5H = 0xFF;	//Counter higher 8-bit value to which OCR5xH value is compared with
	TCNT5L = 0x01;	//Counter lower 8-bit value to which OCR5xH value is compared with
	OCR5AH = 0x00;	//Output compare register high value for Left Motor
	OCR5AL = 0xFF;	//Output compare register low value for Left Motor
	OCR5BH = 0x00;	//Output compare register high value for Right Motor
	OCR5BL = 0xFF;	//Output compare register low value for Right Motor
	OCR5CH = 0x00;	//Output compare register high value for Motor C1
	OCR5CL = 0xFF;	//Output compare register low value for Motor C1
	TCCR5A = 0xA9;	/*{COM5A1=1, COM5A0=0; COM5B1=1, COM5B0=0; COM5C1=1 COM5C0=0}
 					  For Overriding normal port functionality to OCRnA outputs.
				  	  {WGM51=0, WGM50=1} Along With WGM52 in TCCR5B for Selecting FAST PWM 8-bit Mode*/
	
	TCCR5B = 0x0B;	//WGM12=1; CS12=0, CS11=1, CS10=1 (Prescaler=64)
}


//TIMER1 initialization in 10 bit fast PWM mode  
//prescale:256
// WGM: 7) PWM 10bit fast, TOP=0x03FF
// actual value: 52.25Hz 
void timer1_init(void)
{
 TCCR1B = 0x00; //stop
 TCNT1H = 0xFC; //Counter high value to which OCR1xH value is to be compared with
 TCNT1L = 0x01;	//Counter low value to which OCR1xH value is to be compared with
 OCR1AH = 0x03;	//Output compare Register high value for servo 1
 OCR1AL = 0xFF;	//Output Compare Register low Value For servo 1
 OCR1BH = 0x03;	//Output compare Register high value for servo 2
 OCR1BL = 0xFF;	//Output Compare Register low Value For servo 2
 OCR1CH = 0x03;	//Output compare Register high value for servo 3
 OCR1CL = 0xFF;	//Output Compare Register low Value For servo 3
 ICR1H  = 0x03;	
 ICR1L  = 0xFF;
 TCCR1A = 0xAB; /*{COM1A1=1, COM1A0=0; COM1B1=1, COM1B0=0; COM1C1=1 COM1C0=0}
 					For Overriding normal port functionality to OCRnA outputs.
				  {WGM11=1, WGM10=1} Along With WGM12 in TCCR1B for Selecting FAST PWM Mode*/
 TCCR1C = 0x00;
 TCCR1B = 0x0C; //WGM12=1; CS12=1, CS11=0, CS10=0 (Prescaler=256)
}

void adc_init()
{
	ADCSRA = 0x00;
	ADCSRB = 0x00;		//MUX5 = 0
	ADMUX = 0x20;		//Vref=5V external --- ADLAR=1 --- MUX4:0 = 0000
	ACSR = 0x80;
	ADCSRA = 0x86;		//ADEN=1 --- ADIE=1 --- ADPS2:0 = 1 1 0
}

//Function For ADC Conversion
unsigned char ADC_Conversion(unsigned char Ch) 
{
	unsigned char a;
	if(Ch>7)
	{
		ADCSRB = 0x08;
	}
	Ch = Ch & 0x07;  			
	ADMUX= 0x20| Ch;	   		
	ADCSRA = ADCSRA | 0x40;		//Set start conversion bit
	while((ADCSRA&0x10)==0);	//Wait for conversion to complete
	a=ADCH;
	ADCSRA = ADCSRA|0x10; //clear ADIF (ADC Interrupt Flag) by writing 1 to it
	ADCSRB = 0x00;
	return a;
}

//Function To Print Sesor Values At Desired Row And Coloumn Location on LCD
void print_sensor(char row, char coloumn,unsigned char channel)
{
	
	ADC_Value = ADC_Conversion(channel);
	lcd_print(row, coloumn, ADC_Value, 3);
}

//Function for velocity control
void velocity (unsigned char left_motor, unsigned char right_motor)
{
	OCR5AL = (unsigned char)left_motor;
	OCR5BL = (unsigned char)right_motor;
}

//Function used for setting motor's direction
void motion_set (unsigned char Direction)
{
 unsigned char PortARestore = 0;

 Direction &= 0x0F; 		// removing upper nibbel for the protection
 PortARestore = PORTA; 		// reading the PORTA original status
 PortARestore &= 0xF0; 		// making lower direction nibbel to 0
 PortARestore |= Direction; // adding lower nibbel for forward command and restoring the PORTA status
 PORTA = PortARestore; 		// executing the command
}

void forward (void) 
{
  motion_set (0x06);
}

void back (void) //both wheels backward
{
  motion_set(0x09);
}

void left (void) //Left wheel backward, Right wheel forward
{
  motion_set(0x05);
}

void right (void) //Left wheel forward, Right wheel backward
{
  motion_set(0x0A);
}

void soft_left (void) //Left wheel stationary, Right wheel forward
{
 motion_set(0x04);
}

void soft_right (void) //Left wheel forward, Right wheel is stationary
{
 motion_set(0x02);
}

void soft_left_2 (void) //Left wheel backward, right wheel stationary
{
 motion_set(0x01);
}

void soft_right_2 (void) //Left wheel stationary, Right wheel backward
{
 motion_set(0x08);
}

void stop (void)
{
  motion_set (0x00);
}

//Function used for turning robot by specified degrees
void angle_rotate(unsigned int Degrees)
{
 float ReqdShaftCount = 0;
 unsigned long int ReqdShaftCountInt = 0;

 ReqdShaftCount = (float) Degrees/ 4.090; // division by resolution to get shaft count
 ReqdShaftCountInt = (unsigned int) ReqdShaftCount;
 ShaftCountRight = 0; 
 ShaftCountLeft = 0; 

 while (1)
 {
  if((ShaftCountRight >= ReqdShaftCountInt) | (ShaftCountLeft >= ReqdShaftCountInt))
  break;
 }
 stop(); //Stop robot
}

//Function used for moving robot forward by specified distance

void linear_distance_mm(unsigned int DistanceInMM)
{
 float ReqdShaftCount = 0;
 unsigned long int ReqdShaftCountInt = 0;

 ReqdShaftCount = DistanceInMM / 5.338; // division by resolution to get shaft count
 ReqdShaftCountInt = (unsigned long int) ReqdShaftCount;
  
 ShaftCountRight = 0;
 while(1)
 {
 if(ShaftCountRight > ReqdShaftCountInt)
  {
  	break;
  }
 } 
 
 stop(); //Stop robot
}


void forward_mm(unsigned int DistanceInMM)
{
 forward();
 linear_distance_mm(DistanceInMM);
}

void back_mm(unsigned int DistanceInMM)
{
 back();
 linear_distance_mm(DistanceInMM);
}

void left_degrees(unsigned int Degrees) 
{
// 88 pulses for 360 degrees rotation 4.090 degrees per count
 left(); //Turn left
 angle_rotate(Degrees);
}



void right_degrees(unsigned int Degrees)
{
// 88 pulses for 360 degrees rotation 4.090 degrees per count
 right(); //Turn right
 angle_rotate(Degrees);
}


void soft_left_degrees(unsigned int Degrees)
{
 // 176 pulses for 360 degrees rotation 2.045 degrees per count
 soft_left(); //Turn soft left
 Degrees=Degrees*2;
 angle_rotate(Degrees);
}

void soft_right_degrees(unsigned int Degrees)
{
 // 176 pulses for 360 degrees rotation 2.045 degrees per count
 soft_right();  //Turn soft right
 Degrees=Degrees*2;
 angle_rotate(Degrees);
}

void soft_left_2_degrees(unsigned int Degrees)
{
 // 176 pulses for 360 degrees rotation 2.045 degrees per count
 soft_left_2(); //Turn reverse soft left
 Degrees=Degrees*2;
 angle_rotate(Degrees);
}

void soft_right_2_degrees(unsigned int Degrees)
{
 // 176 pulses for 360 degrees rotation 2.045 degrees per count
 soft_right_2();  //Turn reverse soft right
 Degrees=Degrees*2;
 angle_rotate(Degrees);
}

//Function To Initialize UART0
// desired baud rate:9600
// actual baud rate:9600 (error 0.0%)
// char size: 8 bit
// parity: Disabled


void uart0_init(void)
{
 UCSR0B = 0x00; //disable while setting baud rate
 UCSR0A = 0x00;
 UCSR0C = 0x06;
 UBRR0L = 0x5F; //set baud rate lo
 UBRR0H = 0x00; //set baud rate hi
 UCSR0B = 0x98;
}


SIGNAL(SIG_USART0_RECV) 		// ISR for receive complete interrupt
{
	data = UDR0; 				//making copy of data from UDR0 in 'data' variable 

	UDR0 = color; 				//echo data back to PC

		if(data == 0x38) //ASCII value of 8
		{
			PORTA=0x06;  //forward
		}

		if(data == 0x32) //ASCII value of 2
		{
			PORTA=0x09; //back
		}

		if(data == 0x34) //ASCII value of 4
		{
			PORTA=0x05;  //left
		}

		if(data == 0x36) //ASCII value of 6
		{
			PORTA=0x0A; //right
		}

		if(data == 0x35) //ASCII value of 5
		{
			PORTA=0x00; //stop
		}

		if(data == 0x37) //ASCII value of 7
		{
			buzzer_on();
		}

		if(data == 0x39) //ASCII value of 9
		{
			buzzer_off();
		}

}


//Filter Selection
void filter_red(void)    //Used to select red filter
{
	//Filter Select - red filter
	PORTD = PORTD & 0xBF; //set S2 low
	PORTD = PORTD & 0x7F; //set S3 low
}

void filter_green(void)	//Used to select green filter
{
	//Filter Select - green filter
	PORTD = PORTD | 0x40; //set S2 High
	PORTD = PORTD | 0x80; //set S3 High
}

void filter_blue(void)	//Used to select blue filter
{
	//Filter Select - blue filter
	PORTD = PORTD & 0xBF; //set S2 low
	PORTD = PORTD | 0x80; //set S3 High
}

void filter_clear(void)	//select no filter
{
	//Filter Select - no filter
	PORTD = PORTD | 0x40; //set S2 High
	PORTD = PORTD & 0x7F; //set S3 Low
}

//Color Sensing Scaling
void color_sensor_scaling()		//This function is used to select the scaled down version of the original frequency of the output generated by the color sensor, generally 20% scaling is preferable, though you can change the values as per your application by referring datasheet
{
	//Output Scaling 20% from datasheet
	//PORTD = PORTD & 0xEF;
	PORTD = PORTD | 0x10; //set S0 high
	//PORTD = PORTD & 0xDF; //set S1 low
	PORTD = PORTD | 0x20; //set S1 high
}

void red_read(void) // function to select red filter and display the count generated by the sensor on LCD. The count will be more if the color is red. The count will be very less if its blue or green.
{
	//Red
	filter_red(); //select red filter
	pulse=0; //reset the count to 0
//	_delay_ms(100); //capture the pulses for 100 ms or 0.1 second
	red = pulse;  //store the count in variable called red
	
/*	lcd_cursor(1,1);  //set the cursor on row 1, column 1
	lcd_string("Red Pulses"); // Display "Red Pulses" on LCD
	lcd_print(2,1,red,5);  //Print the count on second row
	//_delay_ms(1000);	// Display for 1000ms or 1 second
	lcd_wr_command(0x01); //Clear the LCD */
}

void green_read(void) // function to select green filter and display the count generated by the sensor on LCD. The count will be more if the color is green. The count will be very less if its blue or red.
{
	//Green
	filter_green(); //select green filter
	pulse=0; //reset the count to 0
//	_delay_ms(100); //capture the pulses for 100 ms or 0.1 second
	green = pulse;  //store the count in variable called green
	
	/*lcd_cursor(1,1);  //set the cursor on row 1, column 1
	lcd_string("Green Pulses"); // Display "Green Pulses" on LCD
	lcd_print(2,1,green,5);  //Print the count on second row
	//_delay_ms(1000);	// Display for 1000ms or 1 second
	lcd_wr_command(0x01); //Clear the LCD */
}

void blue_read(void) // function to select blue filter and display the count generated by the sensor on LCD. The count will be more if the color is blue. The count will be very less if its red or green.
{
	//Blue
	filter_blue(); //select blue filter
	pulse=0; //reset the count to 0
//	_delay_ms(100); //capture the pulses for 100 ms or 0.1 second
	blue = pulse;  //store the count in variable called blue
	
/*	lcd_cursor(1,1);  //set the cursor on row 1, column 1
	lcd_string("Blue Pulses"); // Display "Blue Pulses" on LCD
	lcd_print(2,1,blue,5);  //Print the count on second row
	//_delay_ms(1000);	// Display for 1000ms or 1 second
	lcd_wr_command(0x01); //Clear the LCD */
}

//Function to rotate Servo 1 by a specified angle in the multiples of 1.86 degrees
void servo_1(unsigned char degrees)  
{
 float PositionPanServo = 0;
  PositionPanServo = ((float)degrees /1.86) + 35.0;
 OCR1AH = 0x00;
 OCR1AL = (unsigned char) PositionPanServo;
}

//servo_free functions unlocks the servo motors from the any angle 
//and make them free by giving 100% duty cycle at the PWM. This function can be used to 
//reduce the power consumption of the motor if it is holding load against the gravity.

void servo_1_free (void) //makes servo 1 free rotating
{
 OCR1AH = 0x03; 
 OCR1AL = 0xFF; //Servo 1 off
}

//Function To Initialize all The Devices
void init_devices()
{
 cli(); //Clears the global interrupts
 port_init();  //Initializes all the ports
 color_sensor_pin_interrupt_init();
 left_position_encoder_interrupt_init();
 right_position_encoder_interrupt_init();
 adc_init();
 timer1_init();
 timer5_init();
 uart0_init(); //Initailize UART1 for serial communiaction
 sei();   //Enables the global interrupts
}




//Function to initialize ports
void port_init()
{
        servo1_pin_config(); //Configure PORTB 5 pin for servo motor 1 operation
        lcd_port_config();
        adc_pin_config();
	motion_pin_config();
	buzzer_pin_config();
        color_sensor_pin_config();//color sensor pin configuration
       left_encoder_pin_config(); //left encoder pin config
       right_encoder_pin_config(); //right encoder pin config	
}








//Function definition
void analyze_object();
void check_node();
void initialize_motion();
void prepare_motion(int foreward_flag,int left_flag,int right_flag);
void update_path_array(int foreward_flag,int left_flag,int right_flag);
void update_object_array(int object_flag,char color,char side);
int direction2num(char direction);

//Structs used
typedef struct{
	int object_flag;	//Shows if the object is present or notin that cell
	char r_color;		//Give the color of that face(r,g,b,B:black)
	char l_color;		//done in static cordinate as in the picture
	char u_color;
	char d_color;
}object;
typedef struct{
	int r_free;			//done in static cordinate(as in picture)
	int l_free;
	int u_free;
	int d_free;
}node_path;
typedef struct{
	int node_x;			//last node visited ka x_cordinate
	int node_y;			//last node visited ka y_cordinate
	char direction;		//(u,d,l,r) according to our fixed cordinate
	char present;		//(m,n) where m:mid,n:node
}pos_vector;
typedef struct{
	int r;
	int g;
	int b;
}threshold;

void initialize_position(pos_vector *bot_pos);
void update_path_array(int foreward_flag,int left_flag,int right_flag);
//global Variables
object object_array[5][5];
node_path path_array[6][6];
int dist_counter_1=0;
int dist_counter_2=0;
pos_vector bot_pos;
int node_detection(int Left_white_line1, int Center_white_line1,int Right_white_line1);
char check_face_color(int thresh);

int main()
{
int n=0;
init_devices();
lcd_set_4bit();
	lcd_init();
	initialize_position(&bot_pos);	
	int f=0;
	while(1)
	{
		Left_white_line = ADC_Conversion(3);	//Getting data of Left WL Sensor
		Center_white_line = ADC_Conversion(2);	//Getting data of Center WL Sensor
		Right_white_line = ADC_Conversion(1);	//Getting data of Right WL Sensor
		print_sensor(1,5,2);	//Prints Value of White Line Sensor2
        int node=0;  
		flag=0;
		node=node_detection(Left_white_line,Center_white_line,Right_white_line); 
		distance = (int)((ShaftCountRight+ShaftCountLeft)*5.338/2.0);
		
	/*	if(distance>200)
		{
         stop();
		 f=1;
         color=check_face_color(400);
		 _delay_ms(100);
         forward();
		 distance=0;
		 
		} */
		
		if(Center_white_line>0x46)
		{
		//	flag=1;
			forward();
			velocity(130,130);
		}

		if((Left_white_line>0x46) )
		{
			flag=1;
			forward();
			velocity(0,150);
		}

		if((Right_white_line>0x46))
		{
			flag=1;
			forward();
			velocity(150,0);
		}


		/*if(Center_white_line<0x28 && Left_white_line<0x28 && Right_white_line<0x28)
		{
			forward();
			velocity(0,0);
		}*/
		if(node==1)
			{
forward();
velocity(80,80);

			}
			
		
	/*	
		forward_mm(200);
		_delay_ms(100);
		analyze_object(150,200); //object and colour threshold 
		forward_mm(200);
		_delay_ms(100);   
		//double check for node using light sensors, have to use it for shapes
		check_node(450); // THis is Ir threshold for the obstacle detection.
	*/	
	}
	
}

int node_detection(int Left_white_line1, int Center_white_line1,int Right_white_line1)
{
	
	if(Left_white_line1>0x32 && Right_white_line1>0x32)
	{
		return 1;
	}
	if(Left_white_line1>0x32 && Center_white_line1>0x32)
	{
		return 1;
	}
	if(Center_white_line1>0x32 && Right_white_line1>0x32)
	{
		return 1;
	}
	if(Center_white_line1>0x32 && Right_white_line1>0x32 && Left_white_line1>0x32)
	    {
         return 1;
		}

	else 
		return 0;
	
	
}


void initialize_position(pos_vector *bot_pos)
{
    //printf("HI");
	bot_pos->node_x=0;
	bot_pos->node_y=0;
	bot_pos->direction='l';
	bot_pos->present='n';
}

char check_face_color(int thresh)
{
	   red_read(); //display the pulse count when red filter is selected
	   _delay_ms(50);
	   green_read(); //display the pulse count when green filter is selected
	   _delay_ms(50);
	   blue_read(); //display the pulse count when blue filter is selected
	   _delay_ms(50); 
	   if(red<thresh&&blue<thresh&&green<thresh)
		   return 'z';
	   else if(red>blue&&red>green)
		   return 'r';
	   else if(red<blue&&green<blue)
		   return 'b';
	   else if(green>blue&&red<green)
		   return 'g';
	    else
		   return 'q';
}

void analyze_object(int obj_threshold,int colourthresh)
{
	
	int left_ir=0,right_ir=0;
	int left_ir_read=0, right_ir_read=0;

	left_ir_read=ADC_Conversion(4);									//BHAGAT
	right_ir_read=ADC_Conversion(13);								//BHAGAT
	if(left_ir_read< obj_threshold)  //threshold
		left_ir=1;
	if(right_ir_read< obj_threshold) //threshold
		right_ir=1;
	
	if (left_ir)
	{
		servo_1(0);									//BHAGAT
		color=check_face_color(colourthresh);								//BHAGAT
        update_object_array(1,color,'l');
	}
    else{
        update_object_array(0,color,'l');
    }
	if (right_ir)
	{
		servo_1(180);									//BHAGAT
		color=check_face_color(colourthresh);							//BHAGAT
        update_object_array(1,color,'r');
	}
    else{
        update_object_array(0,color,'r');
    }
}


void update_object_array(int object_flag,char color,char side)
{
    int x,y;
    int x_add=0,y_add=0;
    int face_lift_x=0,face_lift_y=0;
    if (bot_pos.direction=='l'){
        x_add=-1;
        if (side=='l'){
            face_lift_y=-1;
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].u_color=color;
        }
        else{
            //printf("%d,%d\n",bot_pos.node_x+x_add+face_lift_x,bot_pos.node_y+y_add+face_lift_y);
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].d_color=color;
            
        }
    }
    else if(bot_pos.direction=='d'){
        y_add=-1;
        if (side=='r'){
            face_lift_x=-1;
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].r_color=color;
        }
        else{
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].l_color=color;
        }
    }
    else if(bot_pos.direction=='u'){
        if (side=='l'){
            face_lift_x=-1; 
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].r_color=color;
        }
        else{
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].l_color=color;
        }
    }
    else{
        if (side=='r'){
            //printf("Hi");
            face_lift_y=-1;
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].u_color=color;
        }
        else{
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].d_color=color;
        }
    }
    
    x=bot_pos.node_x+x_add+face_lift_x;
    y=bot_pos.node_y+y_add+face_lift_y;
    
    //Updating
    if (object_flag==1){
        object_array[x][y].object_flag=1; 
    }
    else{
        object_array[x][y].object_flag=0;
    }
}

void check_node(int obstacle_threshold)
{
	int left_ir_read=0, right_ir_read=0,forward_ir_read=0;
	int left_ir=0,right_ir=0,forward_ir=0;
	forward_ir_read=ADC_Conversion(11);		
	left_ir_read=ADC_Conversion(4);									//BHAGAT
	right_ir_read=ADC_Conversion(13);								//BHAGAT
	if(left_ir_read<obstacle_threshold)  //threshold
		left_ir=1;
	if(right_ir_read<obstacle_threshold) //threshold
		right_ir=1;
	if(forward_ir_read<obstacle_threshold)  //threshold
		forward_ir=1;
	
	
    int foreward_flag=forward_ir;                          //Bot's reference BHAGAT
	int left_flag=left_ir;
    int right_flag=right_ir;
    update_path_array(foreward_flag,left_flag,right_flag);
}

void update_path_array(int foreward_flag,int left_flag,int right_flag){
    char dir=bot_pos.direction;
    int x=bot_pos.node_x;
    int y=bot_pos.node_y;
    if (dir=='l'){
        path_array[x][y].l_free=foreward_flag;
        path_array[x][y].r_free=1;
        path_array[x][y].d_free=left_flag;
        path_array[x][y].u_free=right_flag;
    }
    else if(dir=='r'){
        path_array[x][y].r_free=foreward_flag;
        path_array[x][y].l_free=1;
        path_array[x][y].u_free=left_flag;
        path_array[x][y].d_free=right_flag;
    }
    else if(dir=='u'){
        path_array[x][y].u_free=foreward_flag;
        path_array[x][y].l_free=left_flag;
        path_array[x][y].r_free=right_flag;
        path_array[x][y].d_free=1;
    }
    else if(dir=='d'){
        path_array[x][y].d_free=foreward_flag;
        path_array[x][y].u_free=1;
        path_array[x][y].r_free=left_flag;
        path_array[x][y].l_free=right_flag;
    }
}