#!/usr/bin/env python
import serial
import time
import csv
import datetime
from optparse import OptionParser
from datetime import datetime, date


print("Barntalk started...")
parser = OptionParser()
parser.add_option("-p", "--port", dest="port", default="/dev/ttyACM0",
                  help="Port the arduino is on -- default is /dev/ttyACM0.")
(options, args) = parser.parse_args()

# configuration variables
N_COL_IN_GOOD_ROW = 17  # A valid row should have 17 records in it

timestamps=0
tOutdoor=0
tIndoor=0
tPanel=0
tRoof=0
tInlet=0
tOutlet=0
fanState=0
blowerState=0

labels=[]
entry=[]

timenow=date.strftime(datetime.now(), "%m/%d/%y %H:%M:%S")

#		b=[time.strptime(r[0],"%m/%d/%y %H:%M:%S"),r[2],r[4],r[6],r[8],r[10],r[12],r[14],r[16]]
#		t=time.strptime(r[0],"%m/%d/%y %H:%M:%S");
#print(labels)
#print entry
#print(timestamps)

print(timenow)

#Indoor,40.34,Roof,40.6,Outdoor,31.100,Inlet,37.11,Outlet,39.24,Panel,33.36,CFState,0,BlowerState,0 

ser = serial.Serial('/dev/ttyACM0', 9600, dsrdtr=False, timeout=1)
ser.setDTR()
time.sleep(2)
ser.flush()
ser.flushInput()

#x = ser.read()          # read one byte
#s = ser.read(10)        # read up to ten bytes (timeout)
#line = ser.readline()   # read a '\n' terminated line
ser.write("0W4142434445464748494A4B4C4D4E4F50\n")
#ser.write("0W4A4A4A4A4A4A4B4C4D4E4F\n")
ser.write("0RFF\n")
hello = ser.readline()
print hello
time.sleep(4)
ser.close()

