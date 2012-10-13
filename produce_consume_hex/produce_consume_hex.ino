// produce_consume_hex was written 2012-10-06 my Matt Wortley
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
// To test, fire up the serial terminal and put in something like
// 0W11223344556677889900AABBCCDDEEFF
// Then put in
// 0RFF
// Note that I used ALL CAPS.  MUST BE ALL CAPS.  ONLY CAPS.
// 0 is the address W is the write command 11..FF is the data.
// There are more bytes of data than can go over, so the arduino
// will ignore those.  On the read command we ask for the data
// starting at byte 0 and then as for FF (255) bytes.  It only
// spits out as many as were allocated by #define SHARED_BYTES (14)
// I like to use the FF.  Be sure to check out my barn brain solar
// panel example for how to work with the data in python easily
// and how to post it back to the arduino.
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

#define SHARED_BYTES 14  // How many bytes of shared memory we have
                         // This can be a size of a structure too
#define LINE_BUFF_SZ 60  // How many bytes we can accept at a
                         // single shot before a newline character
#include "string.h"

unsigned char shared_array[SHARED_BYTES];

unsigned long serialdata;
int inbyte;
int digitalState;
int pinNumber;
int analogRate;
int sensorVal;
char tmp_str[LINE_BUFF_SZ];
char lineReadyFlag = 0;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  getSerial();
  if (lineReadyFlag == 1) parseLine(tmp_str);
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
  while(line_str[i]!='\0')
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
        t=isHexChar(line_str[i]);
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
        t=isHexChar(line_str[i]);
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
        if(line_str[i]=='W') 
        {  
          mystate=WRITING;
          i++;
        }
        else if(line_str[i]=='R') 
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
        t=isHexChar(line_str[i]);
        //Serial.print("t was:");
        //Serial.println(t);
        if(t == -1) mystate=ECHOBYTE;
        else
        { 
          i++;
          value=t;
        }
        t=isHexChar(line_str[i]);
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
  line_str[0]='\0'; // put a null character at the start of the line buffer to CLEAR it.
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
   else return eb + '7';
   return 0;
}
