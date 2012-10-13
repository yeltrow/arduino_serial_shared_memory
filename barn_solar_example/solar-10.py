#!/usr/bin/env python
import time
import csv
#import Gnuplot
import datetime
from matplotlib.pyplot import figure, show
from matplotlib.dates import DayLocator, HourLocator, DateFormatter, drange
from numpy import arange
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-s", "--start-date", dest="startDate", default="2/5/12",
                  help="Ignore data with a timestamp before this given start date.  Stamp must be in mm/dd/yyyy format.")
parser.add_option("-e", "--end-date", dest="endDate", default="2/5/32",
                  help="Ignore data with a timestamp after this given start date.")
parser.add_option("-f", "--file", dest="filename", default="/home/wortley/Dcouments/python/solar/barn.csv",
                  help="A file to read in instead of the default.")
parser.add_option("-o", "--outfile", dest="outfile", default="/home/wortley/solarstats.txt",
                  help="A file to append the statistics to.")
parser.add_option("-q", "--quiet",
                  action="store_false", dest="verbose", default=True,
                  help="don't print status messages to stdout")
parser.add_option("-t", "--text-only", 
		  action="store_true", dest="textOnly", default=False,
                  help="Only text to stdout.  No graph generation.")
(options, args) = parser.parse_args()

# configuration variables
N_COL_IN_GOOD_ROW = 17  # A valid row should have 17 records in it
MY_CFM=620
BTU_PER_CFM=0.018
print '[info] Start date: ', options.startDate
t=time.strptime(options.startDate,"%m/%d/%y")
startUTime=(time.mktime(t))

t=time.strptime(options.endDate,"%m/%d/%y")
endUTime=(time.mktime(t))


oldRowUTime=0
timeDifference=0
blowerSecsOn=0
CFSecsOn=0


data=csv.reader(open(options.filename,'rb'), delimiter=',')


i=0
timestamps=[]
tOutdoor=[]
tIndoor=[]
tPanel=[]
tRoof=[]
tInlet=[]
tOutlet=[]
tDifferential = []
timeDifferential = int(0)
timeDifferenceSum = long(0)
tDiffSecs = 0  # The degree difference seconds (used to prevent floating point errors when adding large 
		# accumulated values to small temperature differences
fanState=[]
blowerState=[]
btus=0
tOutletMax=0
tDifferentialMax=0
tIndoorMax=0
tIndoorMin=999
tOutdoorMax=0
tOutdoorMin=999
inOutDifferentialSum = long(0)

labels=[]
entry=[]
for row in data:
	r=row
	i=i+1
	# print(r[0])
	# print('r has a lenghth of', len(r))
	if len(r) == N_COL_IN_GOOD_ROW:
		# we have a good row
		if len(labels) == 0:
			labels=['Date',r[1],r[3],r[5],r[7],r[9],r[11],r[13],r[15]]
		b=[time.strptime(r[0],"%m/%d/%y %H:%M:%S"),r[2],r[4],r[6],r[8],r[10],r[12],r[14],r[16]]
		t=time.strptime(r[0],"%m/%d/%y %H:%M:%S");
		rowUTime=(time.mktime(t))

		if rowUTime > startUTime and rowUTime < endUTime :
			if oldRowUTime == 0:
				firstRowUTime = rowUTime;
			entry.append(b)	
			#timestamps.append((rowUTime-startUTime)/3600)	
			timestamps.append(datetime.datetime.fromtimestamp(rowUTime))
			#timestamps.append(time.strftime("%m/%d/%y %H:%M:%S",t))
			tOutdoorRow=float(r[6])
			tOutdoor.append(tOutdoorRow)
			if tOutdoorRow > tOutdoorMax:
				tOutdoorMax = tOutdoorRow
				tOutdoorMaxTime = rowUTime
			if tOutdoorRow < tOutdoorMin:
				tOutdoorMin = tOutdoorRow
				tOutdoorMinTime = rowUTime
			tIndoorRow=float(r[2])
			tIndoor.append(float(r[2]))
			if tIndoorRow > tIndoorMax:
				tIndoorMax = tIndoorRow
				tIndoorMaxTime = rowUTime
			if tIndoorRow < tIndoorMin:
				tIndoorMin = tIndoorRow
				tIndoorMinTime = rowUTime
			inOutDifferentialRow = tIndoorRow - tOutdoorRow
			inOutDifferentialSum = inOutDifferentialSum + inOutDifferentialRow
			tPanel.append(float(r[12]))
			tPanelRow=float(r[12])
			tRoof.append(float(r[4]))
			tInletRow=float(r[8])
			tInlet.append(tInletRow)
			tOutletRow=float(r[10])
			tOutlet.append(tOutletRow)
			if tOutletRow > tOutletMax:
				tOutletMax = tOutletRow
				tOutletMaxTime = rowUTime
			indoorPanelDifferential=tPanelRow - tIndoorRow
			tDifferentialRow=tOutletRow - tInletRow
			if oldRowUTime :
				timeDifference=rowUTime-oldRowUTime
				timeDifferenceSum = timeDifferenceSum + timeDifference
			tDifferential.append(tDifferentialRow)
			if tDifferentialRow > tDifferentialMax:
				tDifferentialMax = tDifferentialRow
				tDifferentialMaxTime = rowUTime
			CFStateRow=int(r[14])
			fanState.append(CFStateRow+2)
			if CFStateRow == 1 :
				CFSecsOn=CFSecsOn+int(round(timeDifference,0))
			rowBlowerState=int(r[16])
			blowerState.append(rowBlowerState)
			if rowBlowerState == 1 :
				blowerSecsOn=blowerSecsOn+int(round(timeDifference,0))
				tDiffSecs = tDiffSecs + round(tDifferentialRow,0)*timeDifference
			oldRowUTime=rowUTime
			#entry[len(entry):]=b
	else:
		print '[warning] Bad row found at line: ',i
		print r
		
#print(labels)
#print entry
#print(timestamps)
print '[info] Nuber of rows processed is:', len(entry)
nDays = (rowUTime - firstRowUTime)/24/3600
print '[info] Records covered ', nDays , 'days. '
print '[stat] Outlet Maximum Temperature: ', tOutletMax, 'was recorded:', time.strftime("%m/%d/%y %H:%M:%S", time.localtime(tOutletMaxTime))
print '[stat] Inlet/Outlet Max Differential: ', tDifferentialMax, 'was recorded:', time.strftime("%m/%d/%y %H:%M:%S", time.localtime(tDifferentialMaxTime))
print '[stat] Indoor Max Temp: ', tIndoorMax, 'was recorded:', time.strftime("%m/%d/%y %H:%M:%S", time.localtime(tIndoorMaxTime))
print '[stat] Indoor Min Temp: ', tIndoorMin, 'was recorded:', time.strftime("%m/%d/%y %H:%M:%S", time.localtime(tIndoorMinTime))
print '[stat] Outdoor Max Temp: ', tOutdoorMax, 'was recorded:', time.strftime("%m/%d/%y %H:%M:%S", time.localtime(tOutdoorMaxTime))
print '[stat] Outdoor Min Temp: ', tOutdoorMin, 'was recorded:', time.strftime("%m/%d/%y %H:%M:%S", time.localtime(tOutdoorMinTime))
print '[stat] Average In/Outdoor Differential: ', inOutDifferentialSum/len(entry)
btus=tDiffSecs/60*MY_CFM*BTU_PER_CFM
print '[stat] Total BTUs: ',btus
kWhEquivalent = btus/3412
blowerOnHours = float(blowerSecsOn)/3600
blowerPowerkWh = blowerOnHours * 0.5
blowerPowerCost = blowerPowerkWh*0.11

print '[stat] Total KWh equivalent: ', kWhEquivalent
heatValue = kWhEquivalent*0.11
print '[stat] Heat Value in US Dollars: ', heatValue
print '[stat] Average heat value in $/day: ',heatValue/nDays
print '[stat] Total Blower On Time(hours): ',blowerOnHours
print '[stat] Cost to run blower(USD): ',blowerPowerCost
CFOnHours = float(CFSecsOn)/3600
CFPowerkWh = CFOnHours * 0.120
CFPowerCost = CFPowerkWh*0.11
print '[stat] Total Ceiling Fan On Time(hours): ',CFOnHours
print '[stat] Cost to Ceiling Fans (USD): ',CFPowerCost
if (blowerPowerCost + CFPowerCost) > 0:
	DollarsOutvsIn = (heatValue + blowerPowerCost + CFPowerCost)/(blowerPowerCost + CFPowerCost)
	print '[stat] Ratio of Heat Delivered to Power Consumed (USD): ',DollarsOutvsIn
#ans = raw_input('Enter to quit ')
#g('set xtics rotate')
#g('show xtics')


date1 = datetime.datetime( 2000, 3, 2)
date2 = datetime.datetime( 2000, 3, 3)
delta = datetime.timedelta(hours=6)
#dates = drange(date1, date2, delta)
dates=timestamps

y = arange( len(dates)*1.0)

if not options.textOnly:
	fig = figure()
	ax = fig.add_subplot(111)
	l1 = ax.plot_date(dates, tOutdoor, 'go')
	l3 = ax.plot_date(dates, tDifferential, 'yx')
	l4 = ax.plot_date(dates, tInlet, 'b+')
	l5 = ax.plot_date(dates, tOutlet, 'c4')
	l6 = ax.plot_date(dates, tRoof, 'mH')
	l7 = ax.plot_date(dates, tPanel, 'k,')
	l8 = ax.plot_date(dates, blowerState, 'b,')
	l2 = ax.plot_date(dates, tIndoor, 'rs-')
	l9 = ax.plot_date(dates, fanState, 'b,')
	# this is superfluous, since the autoscaler should get it right, but
	# use date2num and num2date to to convert between dates and floats if
	# you want; both date2num and num2date convert an instance or sequence
	ax.set_xlim( dates[0], dates[-1] )

	# The hour locator takes the hour or sequence of hours you want to
	# tick, not the base multiple

	ax.xaxis.set_major_locator( HourLocator(interval=2) )
	ax.xaxis.set_major_formatter( DateFormatter('%Y-%m-%d %H:00') )

	ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')

	fig.autofmt_xdate()
#	fig.legend((l1, l2, l3, l4, l5, l6, l7, l8, l9), ('Outdoor','Indoor','Differential','Inlet', 'Outlet', 'Roof', 'Panel', 'Blower', 'Ceiling Fans'), 'lower right')
	show()




