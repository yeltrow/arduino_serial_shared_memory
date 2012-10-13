#!/usr/bin/env python
import socket # needed to figure out the rasberry Pi's IP and send it to the shared mem space
import serial
import time
import csv
import datetime
from optparse import OptionParser
from datetime import datetime, date


# print("Barntalk started...")
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

# figure out our IP address and put it into some bytes
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("gmail.com",80))
ip_str=(s.getsockname()[0])
s.close()
ip_list=ip_str.split('.')
ip_int=[]
for item in ip_list:
  ip_int.append(int(item))


ser = serial.Serial('/dev/ttyACM0', 9600, dsrdtr=False, timeout=1)
ser.setDTR()
time.sleep(2)
ser.flush()
ser.flushInput()

#x = ser.read()          # read one byte
#s = ser.read(10)        # read up to ten bytes (timeout)
#line = ser.readline()   # read a '\n' terminated line

#
#  A bit about how data is packed in the arduino.  Bytes 0-3 contain the IP address of the
#  RaspberryPi host and are sent to it by this script.
#
temp_data_block_start = 4  # temperate data starts in the fourth byte

send_str="0W"

send_str=send_str+("%.2X" % (ip_int[0]))+("%.2X" % (ip_int[1]))+("%.2X" % (ip_int[2]))+("%.2X" % (ip_int[3]))+"\n"
#print "Ip address should be",
#print ip_int,
#print send_str

ser.write(send_str)

time.sleep(5)
#ser.write("0W41424344454647FF494A4B4C4D4E4F50\n")
# Contains the test temp sequence 100 101 102 103 104 105 CF On Blower On 
ser.write("4W10277427D8273C28A028042903\n")
while 1:
  ts=date.strftime(datetime.now(), "%m/%d/%y %H:%M:%S")
  time.sleep(1)
  ser.flushInput()
  ser.write("4RFF\n")
  time.sleep(1)
  hexstring = ser.readline()
  #print hexstring
  #print "Hexstring Length is:", len(hexstring)
  bytelist = map(ord, hexstring[:-2].decode("hex"))  #we need to strip the extra character off the end
  #print bytelist
  #Indoor,40.34,Roof,40.6,Outdoor,31.100,Inlet,37.11,Outlet,39.24,Panel,33.36,CFState,0,BlowerState,0 
  di = 0
  tIndoor=(bytelist[di]+bytelist[di+1]*0x100)*0.01
  ts=ts+",Indoor,"+str(tIndoor)
  di = di + 2 
  tRoof=(bytelist[di]+bytelist[di+1]*0x100)*0.01
  ts=ts+",Roof,"+str(tRoof)
  di = di + 2 
  tOutdoor=(bytelist[di]+bytelist[di+1]*0x100)*0.01
  ts=ts+",Outdoor,"+str(tOutdoor)
  di = di + 2 
  tInlet=(bytelist[di]+bytelist[di+1]*0x100)*0.01
  ts=ts+",Inlet,"+str(tInlet)
  di = di + 2 
  tOutlet=(bytelist[di]+bytelist[di+1]*0x100)*0.01
  ts=ts+",Outlet,"+str(tOutlet)
  di = di + 2 
  tPanel=(bytelist[di]+bytelist[di+1]*0x100)*0.01
  ts=ts+",Panel,"+str(tPanel)
  di = di + 2 
  CFState=(bytelist[di] & 0x01)
  ts=ts+",CFState,"+str(CFState)
  BlowerState=(bytelist[di] & 0x02)>>1
  ts=ts+",BlowerState,"+str(BlowerState)

  print ts
  if (( datetime.today().hour > 20 ) | (datetime.today().hour < 14 )):
    time.sleep(4)
  else:
    time.sleep(1)

ser.close()


