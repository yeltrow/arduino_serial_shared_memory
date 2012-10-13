// this program was written 2012-10-06 my Matt Wortley
// It accepts input from the serial port to access a shared array
// of bytes (unsigned char).  They are accessed by a very simple
// protocol with no white space:  [address][command][data]
// [address] is the index in the shared array in ascii hex (0A=10dec)
// [command] is either a single CAPITAL W or R for write or read
// [data] is a series of hexadecimal pairs with no white space
// For write commands, each hexadecimal pair causes a write to the
// next memory location. To write three value A0 B1 C2 to the
// seventh through ninth bytes of the shared array use
// something like this:
// 7WA0B1C2 
// THIS MUST END WITH A NEWLINE CHARACTER FOR THE LINE TO BE READ 
// READING EXAMPLE:
// 7R03
// Will produce
// A0B1C2 
// as a reply on the serial line.
// REMEBER YOU ARE USING HEXADECIMAL FOR ALL COMMUNICATIONS
// 10 in HEX is the same a 16 DECIMAL
//

/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <Time.h>

// LCD STUFF ** LCD STUFF ** LCD STUFF ** LCD STUFF ** LCD STUFF ** 
//
// Circuit:  VSS + RW (LCD 1 + 5) to GND (Arduino)
//           VDD (LCD 2) to +5V (Arduino)
//           VO (LCD 3) to Potentiometer
//           RW (LCD 4) to Arduino Digital pin 
//            LCD 5 Skip
//            EN(Enable LCD 6) to Arduino Digital pin 
//            LCD D4 to Arduino Digital pin 
//            LCD D5 to Arduino Digital pin 
//            LCD D6 to Arduino Digital pin 
//            LCD D7 to Arduino Digital pin 
//
//
// include the library code:
// initialize the library with the numbers of the interface pins
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // Barn Brain LCD
//LiquidCrystal lcd(32, 30, 28, 26, 24, 22); // Big LCD
// END LCD STUFF ** END LCD STUFF ** END LCD STUFF ** END LCD STUFF ** 

// Below is the code I stole to convert ADC reads to temperatures
/* ADC = T/(R+T)*1023
   ADC(R+T) = T*1023
   ADC*R + ADC*T = T*1023
   ADC*R = T*1023 - ADC*T
   ADC*R = (1023 - ADC) T
   T = (ADC * R)/(1023 - ADC)
   
   // Resistance = (RawADC * pad)/(1023 - RawADC);
*/

#define SHARED_BYTES 18  // Tells us how many bytes we can share with 
		         // people over the serial port.
#define FAN_RELAY_PIN 10
#define BLOWER_ON_INPUT_PIN 9
uint32_t fanOffTime = 0;
#define FAN_OFF_DIFF_TEMP 1
#define FAN_ON_DIFF_TEMP 3
// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 10
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000   
#define LOOP_TIME 2000

// Global variables
  float steinhart;
  float Toutdoor;
  float Tindoor;
  float Tpanel;
  float Tinlet;
  float Toutlet;
  float Troof;
  unsigned long old_millis;

  #define LINE_BUFF_SZ 60
  unsigned char shared_array[SHARED_BYTES]; 
  int digitalState;
  int pinNumber;
  int analogRate;
  int sensorVal;
  char tmp_str[LINE_BUFF_SZ];
  char lineReadyFlag = 0;


// End Global Variables
 
int samples[NUMSAMPLES];

 
void setup(void) {
  pinMode(FAN_RELAY_PIN, OUTPUT); 
  pinMode(BLOWER_ON_INPUT_PIN, INPUT);
  Serial.begin(9600);
  old_millis=millis()/LOOP_TIME;
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("BARN BRAIN v0.2");
}

unsigned char fanRelayState;
unsigned char blowerOnState;

void loop(void) {  
  // ** SERIAL INTERFACE STUFF **
  getSerial();  // Go check on the serial port and load the line buffer if we need to
  if (lineReadyFlag == 1) // if we go serial data, go handle it.
  {
    parseLine(tmp_str);
    shared_array[SHARED_BYTES - 1] ='\0'; // Put the null character on the end 
  }
  // ** END SERIAL INTERFACE STUFF **

  read_all_temp_sensors();    
  // serial_print_temp_sensors();
  // Handle the slow periodic tasks like updating the LCD
  // and checking on the blower.
  if ((millis()/LOOP_TIME)!=old_millis) 
  {
    lcd_display_temp_status();
    old_millis=millis()/LOOP_TIME;
    blowerOnState=digitalRead(BLOWER_ON_INPUT_PIN); 
    fanRelayState = (millis()<fanOffTime);
    shared_array[16]=(blowerOnState<<1)+(fanRelayState);
    if(Troof >= (Tindoor + (FAN_OFF_DIFF_TEMP+FAN_ON_DIFF_TEMP -fanRelayState*(FAN_ON_DIFF_TEMP - FAN_OFF_DIFF_TEMP))))
    {
      fanOffTime = millis()+900000L;
    }
    digitalWrite(FAN_RELAY_PIN, fanRelayState);
  }
}

void read_all_temp_sensors(void)
{
  int *pInt; // A pointer to an integer
  Toutdoor = read_thermistor(A0);
  // by using an integer pointer, we can just point
  // it to the right spot and C will automatically
  // convert the float*100 to an int and pack it
  // properly.
  pInt = (int *)&shared_array[8];
  *pInt = Toutdoor*100;
  Tindoor = read_thermistor(A1);
  pInt = (int *)&shared_array[4];
  *pInt = Tindoor*100;
  Tpanel = read_thermistor(A2);
  pInt = (int *)&shared_array[14];
  *pInt = Tpanel*100;
  Tinlet  = read_thermistor(A3);
  pInt = (int *)&shared_array[10];
  *pInt = Tinlet*100;
  Toutlet = read_thermistor(A4);
  pInt = (int *)&shared_array[12];
  *pInt = Toutlet*100;
  Troof  = read_thermistor(A5);
  pInt = (int *)&shared_array[6];
  *pInt = Troof*100;

}

float read_thermistor(unsigned int channel)
{  
  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(channel);
  delay(1);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;

  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  steinhart = 1.8*steinhart + 32;
  return steinhart;
}

void serial_print_temp_sensors(void)
{
  Serial.println("=============================");  
  Serial.print("Temperature Outdoors "); 
  Serial.print(Toutdoor);
  Serial.println(" *F");
  Serial.print("Temperature Indoors "); 
  Serial.print(Tindoor);
  Serial.println(" *F");
  Serial.print("Temperature at Inlet "); 
  Serial.print(Tinlet);
  Serial.println(" *F");
  Serial.print("Temperature at Outlet "); 
  Serial.print(Toutlet);
  Serial.println(" *F");
  Serial.print("Temperature inside Panel "); 
  Serial.print(Tpanel);
  Serial.println(" *F");
  Serial.print("Temperature at roof peak "); 
  Serial.print(Troof);
  Serial.println(" *F");
  delay(1000);
}

void lcd_display_temp_status(void)
{
  // which pass through the routine we are on.  Used to display multiple temps.
  static int pass=0;

  switch (pass)
  {
  case 0:  
    lcd.clear();
    lcd.print("OUTDOOR ");
    lcd.setCursor(8, 0);
    lcd.print(Toutdoor);
    lcd.setCursor(13, 0);
    lcd.print(" *F"); 
    lcd.setCursor(0, 1);
    lcd.print("INDOOR");
    lcd.setCursor(8, 1);
    lcd.print(Tindoor);
    lcd.setCursor(13, 1);
    lcd.print(" *F"); 
    pass=1;
    break;
  case 1:
    lcd.clear();
    lcd.print("INLET ");
    lcd.setCursor(8, 0);
    lcd.print(Tinlet);
    lcd.setCursor(13, 0);
    lcd.print(" *F"); 
    lcd.setCursor(0, 1);
    lcd.print("OUTLET");
    lcd.setCursor(8, 1);
    lcd.print(Toutlet);
    lcd.setCursor(13, 1);
    lcd.print(" *F"); 
    pass=2;
    break;
  case 2:
    lcd.clear();
    lcd.print("INDOOR ");
    lcd.setCursor(8, 0);
    lcd.print(Tindoor);
    lcd.setCursor(13, 0);
    lcd.print(" *F"); 
    lcd.setCursor(0, 1);
    lcd.print("PEAK");
    lcd.setCursor(8, 1);
    lcd.print(Troof);
    lcd.setCursor(13, 1);
    lcd.print(" *F"); 
    pass=3;
    break;
  case 3:
    lcd.clear();
    lcd.print("PANEL");
    lcd.setCursor(8, 0);
    lcd.print(Tpanel);
    lcd.setCursor(13, 0);
    lcd.print(" *F"); 
    lcd.setCursor(0, 1);
    lcd.print("Tout-Tin");
    lcd.setCursor(8, 1);
    lcd.print(Toutlet-Tinlet);
    lcd.setCursor(13, 1);
    lcd.print(" *F"); 
    pass=4;
    break;
  case 4:  // Show the ip address of the host PC
    lcd.clear();
    lcd.print("IP Address");
    lcd.setCursor(0, 1);
    lcd.print(shared_array[0]);
    lcd.print(".");
    lcd.print(shared_array[1]);
    lcd.print(".");
    lcd.print(shared_array[2]);
    lcd.print(".");
    lcd.print(shared_array[3]);
    pass=0;
    break;
  default:
    pass=0;
    break;
  } // end switch pass
}

int getSerial()
{
  static unsigned char chari=0; // character index
  if(lineReadyFlag == 0)
  {
    if(Serial.available())
    {
      tmp_str[chari]=Serial.read();
      tmp_str[chari+1]='\0'; // place the null string terminator at the end every time
      if(tmp_str[chari]=='\n')
      {
        chari=0; // reset the index
        lineReadyFlag=1; // tell the main routine we have data for it.
      }
      else 
      {
         if(chari < (LINE_BUFF_SZ - 2)) // we can only index to the size of the buffer -2 and
         {                                // still have room for the next character and the NULL
            chari++;  
         }
         //Serial.println(tmp_str);
      } // End else
    } // end if(Serial.available())
  } // END if(lineReadyFlag == 0)
  return 0;
}

/*
**  parses a text line when we get one.  Triggers any needed
**  actions like copying it to memory or loading the output
**  buffer with text
*/

enum parseLineState { INITIALIZING, READADDRESS, READCOMMAND, READING, WRITING, READQUANTITY, ECHOBYTE, READINGDATA, IDLE };

void parseLine (char *line_str)
{
  int n;
  int quantity=0;  // how many bytes should be read out if we read
  int value=0; // the temp storage of values we write in
  int t;
  int address=0;
  int i=0;  // character index through the line string
  // Serial.print("parseLine called...");
  enum parseLineState mystate = IDLE;
  while(tmp_str[i]!='\0')
  {
    switch(mystate)
    {  
      case IDLE: 
        //Serial.println("parseLine:IDLE state...");
        mystate=READADDRESS; // always jumps from IDLE to READADDRESS
        break;
      case READADDRESS:
        //Serial.println("parseLine:READADDRESS state...");
        // if done exit to read command
        t=isHexChar(tmp_str[i]);
        //Serial.print("t was:");
        //Serial.println(t);
        if(t == -1) mystate=READCOMMAND;
        else
        { 
          i++;
          address=address*16 + t;
        }
        break;
      case READQUANTITY:
        //Serial.println("parseLine:READQUANTITY state...");
        // if done exit to read command
        t=isHexChar(tmp_str[i]);
        //Serial.print("t was:");
        //Serial.println(t);
        if(t == -1) mystate=READING;
        else
        { 
          i++;
          quantity=quantity*16 + t;
        }
        break;
      case READCOMMAND:
        //Serial.println("parseLine:READCOMMAND state...");
        //Serial.print("Address was:");
        //Serial.println(address);  
        if(tmp_str[i]=='W') 
        {  
          mystate=WRITING;
          i++;
        }
        else if(tmp_str[i]=='R') 
        {  
          mystate=READQUANTITY;
          quantity=0;
          i++;
        }
        else 
        {
        //   Serial.println("BOTCHED COMMAND FOUND!"); 
           i++;
        }
        break;
      case WRITING:
        //Serial.println("parseLine:WRITING state...");
        // read a number from the port
        t=isHexChar(tmp_str[i]);
        //Serial.print("t was:");
        //Serial.println(t);
        if(t == -1) mystate=ECHOBYTE;
        else
        { 
          i++;
          value=t;
        }
        t=isHexChar(tmp_str[i]);
        //Serial.print("t was:");
        //Serial.println(t);
        if(t == -1) mystate=ECHOBYTE;
        else
        { 
          i++;
          value=value*16 + t;
          if(address < SHARED_BYTES)
          {
            shared_array[address]=value;
            address++;
          }
        }
        break;
      case READING:
      //Serial.println("parseLine:READING state...");
        for(n=0;n<quantity;n++)
        {
           if((address+n) < SHARED_BYTES)
           {
             value=shared_array[address+n];
             Serial.print(byte2hexChar(value>>4));
             Serial.print(byte2hexChar(value & 0x0F));
           }
        }
        Serial.println("");
        mystate=ECHOBYTE;
        break;
      case ECHOBYTE:
        //Serial.println("parseLine:ECHOBYTE state...");
        i++;
        break;
      default:
        break;
     }  // end switch
  } // end while 
  tmp_str[0]='\0'; // put a null character at the start of the line buffer to CLEAR it.
  lineReadyFlag = 0; // flag that we have processed the line and are ready for more.
  //Uncomment to see what the shared memory contains on your serial terminal.
  //Serial.println("consumed array:");
  //for (value=0;value<SHARED_BYTES;value++)
  //{
  //  Serial.println(shared_array[value]);
  //}
}


/* isHexChar returns -1 if it isn't a good CAPITAL LETTER HEX CODE */
/* It returns a number 0 or greater if it is a good hex digit */
int isHexChar( char character)
{
     if(character < '0') 
     {
        return -1; // character was lower than a valid hex input 
     }
     else if(character < ':')  // Must be a valid number if we got here
     {
        return character - '0';
     }
     else if(character < 'A')
     {
        return -1;  // Must be between 9 and A -- Not good
     }
     else if( character < 'G') 
     {
       return character - '7'; // Seven is 10 characters less than letter A
     }
     return -1;
}

char byte2hexChar (int eb)
{
   if(eb<0x0A) return eb +'0';
   else return eb + '7'; // Add the character '7' to make sure a value of 0xA produces an ASCII A.
   return 0;
}
