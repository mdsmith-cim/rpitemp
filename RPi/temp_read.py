
from tkinter import *
from tkinter import ttk
import sched, time
from datetime import datetime
import os
import re
import time
from html_sanitizer import Sanitizer
from apscheduler.schedulers.background import BackgroundScheduler
import requests
import csv

non_decimal = re.compile(r'[^\d.]+')

def quitProgram():
	csvfile.close()
	scheduler.shutdown(False)
	print("Exiting...")
	root.destroy()

def removeNonDecimal(text):
	return non_decimal.sub('', text)

#def toggleF():
#	global faren
#	global scheduler
#	faren = not(faren)
#	forceUpdate()

def forceUpdate():
	scheduler.add_job(update_temp)

def updateTime():
	timeDisp.set(time.strftime("%d/%m/%y %l:%M:%S %p"))

def temp_to_format_str(t):
	#if faren:
	#	return str(round(9.0/5.0 * t + 32,2)) + " °F"
	#else:
	return str(round(t,2)) + " °C"

def update_temp():

	# Get local cottage temp
	f = open('/sys/bus/w1/devices/28-000007171178/w1_slave', 'r')
	cot = f.readline()
	if "YES" not in cot:
		raise Exception("Bad CRC for Cottage temp sensor")
		
	cot = f.readline()
	cot = cot.split("t=")
	cot = cot[1].rstrip("\n")

	cot = temp_to_format_str(float(cot)/1000)

	f.close()

	# Get local cottage temp and humidity from DHT

	f = open('/sys/bus/iio/devices/iio:device0/in_temp_input', 'r')
	success = False
	tries = 0
	while not(success):
		try:
			cot2 = temp_to_format_str(float(f.readline())/1000)
			success = True
		except OSError as e:
			tries = tries + 1
			if tries > 10:
				cot2 = "ERR"
				break
	
	f.close()

	f = open('/sys/bus/iio/devices/iio:device0/in_humidityrelative_input', 'r')
	success = False
	tries = 0
	while not(success):
		try:
			hm = str(round(float(f.readline())/1000,2)) + " %"
			success = True
		except OSError as e:
			tries = tries + 1
			if tries > 10:
				hm = "ERR"
				break
	
	f.close()

	tempCottage.set(cot)
	tempCottage2.set(cot2)	
	humid.set(hm)

	# Get info from pumphouse
	r = requests.get("http://192.168.0.180/temp")
	# Set encoding to UTF-8 since the HTML is in it but apparently the ESP sends a different one
	r.encoding = "UTF-8"
	
	# Throw error if applicable
	if not(r.ok):
		lakeTempVar = "ERR"
		outsideTempVar = "ERR"
		intakeTempVar = "ERR"
		ambientTempVar = "ERR"

	else:

		# Clean up and split response
		sanitizer = Sanitizer()
		txt = sanitizer.sanitize(r.text)

		# Split on line breaks and remove all but temp info
		txtSplit = txt.split("<br>")[1:5]
		
		for item in txtSplit:
			tempS = item.split(":")
			tmpValue = float(removeNonDecimal(tempS[1]))
			if "Lake" == tempS[0]:
				lakeTempVar = temp_to_format_str(tmpValue)
			elif "Outside" == tempS[0]:
				outsideTempVar = temp_to_format_str(tmpValue)
			elif "Pump intake" == tempS[0]:
				intakeTempVar = temp_to_format_str(tmpValue)
			elif "Ambient outdoor" == tempS[0]:
				ambientTempVar = temp_to_format_str(tmpValue)

	tempLake.set(lakeTempVar)
	tempOutside.set(outsideTempVar)
	tempIntake.set(intakeTempVar)
	tempAmb.set(ambientTempVar)

	csvwriter.writerow({'Time': time.strftime("%F %T"), 'Outside Temp': removeNonDecimal(outsideTempVar), 'Cottage Temp': removeNonDecimal(cot), 'Cottage Temp (DHT)': removeNonDecimal(cot2), 'Lake Temp': removeNonDecimal(lakeTempVar), 'Pump Intake': removeNonDecimal(intakeTempVar), 'Ambient Lake Temp': removeNonDecimal(ambientTempVar), 'Humidity': removeNonDecimal(hm)})

	
csvfile = open('sensor_log_' + time.strftime("%F_%T") + '.csv', 'w', newline='')
fieldnames = ['Time', 'Outside Temp', 'Cottage Temp', 'Cottage Temp (DHT)', 'Lake Temp', 'Pump Intake','Ambient Lake Temp','Humidity']
csvwriter = csv.DictWriter(csvfile, fieldnames)
csvwriter.writeheader()

root = Tk()
root.title("Sensors")
root.attributes('-fullscreen', True)

mainframe = ttk.Frame(root, padding="12 12 12 12")
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
mainframe.columnconfigure(0, weight=1)
mainframe.rowconfigure(0, weight=1)

tempOutside = StringVar()
tempCottage = StringVar()
tempLake = StringVar()
tempIntake = StringVar()
tempAmb = StringVar()
tempCottage2 = StringVar()
humid = StringVar()
timeDisp = StringVar()

ttk.Label(mainframe, text="Temperature:", font=("Arial", 32)).grid(column=1, row=1, sticky=(N,E,W,S), columnspan=2)

ttk.Label(mainframe, text="Outside", font=("Arial", 32)).grid(column=1, row=2, sticky=S)
ttk.Label(mainframe, text="Water", font=("Arial", 32)).grid(column=2, row=2, sticky=S)
ttk.Label(mainframe, text="Pump Intake", font=("Arial", 32)).grid(column=1, row=4, sticky=S)
ttk.Label(mainframe, text="Cottage", font=("Arial", 32)).grid(column=2, row=4, sticky=S)
ttk.Label(mainframe, text="Outside (Lake)", font=("Arial", 32)).grid(column=1, row=6, sticky=S)
ttk.Label(mainframe, text="Cottage 2", font=("Arial", 32)).grid(column=2, row=6, sticky=S)
ttk.Label(mainframe, text="Time:", font=("Arial", 32)).grid(column=3, row=1, sticky=(N,E,W,S))

ttk.Label(mainframe, text="Humidity (inside):", font=("Arial", 32)).grid(column=1, row=8, sticky=(N,E,W,S), columnspan=2)

ttk.Label(mainframe, textvariable=tempOutside, font=("Arial", 32)).grid(column=1, row=3, sticky=S)
ttk.Label(mainframe, textvariable=tempLake, font=("Arial", 32)).grid(column=2, row=3, sticky=S)
ttk.Label(mainframe, textvariable=tempIntake, font=("Arial", 32)).grid(column=1, row=5, sticky=S)
ttk.Label(mainframe, textvariable=tempCottage, font=("Arial", 32)).grid(column=2, row=5, sticky=S)
ttk.Label(mainframe, textvariable=tempAmb, font=("Arial", 32)).grid(column=1, row=7, sticky=S)
ttk.Label(mainframe, textvariable=tempCottage2, font=("Arial", 32)).grid(column=2, row=7, sticky=S)
ttk.Label(mainframe, textvariable=humid, font=("Arial", 32)).grid(column=1, row=9, sticky=S)

# Update button
ttk.Button(mainframe, text="Force update", command=forceUpdate).grid(column=1, row=10, stick=(W,S), columnspan=2)

# Quit button
ttk.Button(mainframe, text="Quit", command=quitProgram).grid(column=1, row=11, stick=(W,S))

# Time
ttk.Label(mainframe, textvariable=timeDisp, font=("Arial", 32)).grid(column=3, row=2, sticky=(N,E,W,S))

# Fahrenheit checkbox
#global faren
#faren = False
#ttk.Checkbutton(mainframe, text='Fahrenheit', command=toggleF).grid(column=2, row=11, stick=(E,S))

# Add padding to all items
for child in mainframe.winfo_children(): child.grid_configure(padx=5, pady=5)

# Create job scheduler
global scheduler
scheduler = BackgroundScheduler()

# Set job to run once immediately
scheduler.add_job(update_temp)
# Schedule every 5 minute execution
scheduler.add_job(update_temp, 'interval', coalesce=True, minutes=5)

# Schedule time update
scheduler.add_job(updateTime, 'interval', coalesce=True, seconds=1)

# Start scheduler
scheduler.start()

try:
	# Execute main GUI loop
	root.mainloop()
except (KeyboardInterrupt, SystemExit):
	quitProgram()