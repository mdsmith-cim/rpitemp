
from tkinter import *
from tkinter import ttk
import sched, time
from datetime import datetime
import os
import re
from html_sanitizer import Sanitizer
from apscheduler.schedulers.background import BackgroundScheduler
import requests

non_decimal = re.compile(r'[^\d.]+')

def toggleF():
	global faren
	global scheduler
	faren = not(faren)
	forceUpdate()

def forceUpdate():
	scheduler.add_job(update_temp)

def temp_to_format_str(t):
	if faren:
		return str(round(9.0/5.0 * t + 32,2)) + " °F"
	else:
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

	f.close()

	# Get local cottage temp and humidity from DHT

	f = open('/sys/bus/iio/devices/iio:device0/in_temp_input', 'r')
	success = False
	tries = 0
	while not(success):
		try:
			cot2 = f.readline()
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
			hm = f.readline()
			success = True
		except OSError as e:
			tries = tries + 1
			if tries > 10:
				hm = "ERR"
				break
	
	f.close()
	
	# Get info from pumphouse
	r = requests.get("http://192.168.0.180/temp")
	# Set encoding to UTF-8 since the HTML is in it but apparently the ESP sends a different one
	r.encoding = "UTF-8"
	
	# Throw error if applicable
	if not(r.ok):
		tempLake.set("ERR")
		tempOutside.set("ERR")
		tempIntake.set("ERR")
		tempAmb.set("ERR")
	else:

		# Clean up and split response
		sanitizer = Sanitizer()
		txt = sanitizer.sanitize(r.text)

		# Split on line breaks and remove all but temp info
		txtSplit = txt.split("<br>")[1:5]
		
		if cot == "ERR":
			tempCottage.set("ERR")
		else:
			tempCottage.set( temp_to_format_str(float(cot)/1000) )
		if cot2 == "ERR":
			tempCottage2.set("ERR")	
		else:
			tempCottage2.set( temp_to_format_str(float(cot2)/1000))
		if hm == "ERR":
			humid.set("ERR")
		else:
			humid.set(str(round(float(hm)/1000,2)) + " %")
		
		for item in txtSplit:
			tempS = item.split(":")
			tmpValue = non_decimal.sub('', tempS[1])
			if "Lake" == tempS[0]:
				tempLake.set(temp_to_format_str(float(tmpValue)))
			elif "Outside" == tempS[0]:
				tempOutside.set(temp_to_format_str(float(tmpValue)))
			elif "Pump intake" == tempS[0]:
				tempIntake.set(temp_to_format_str(float(tmpValue)))
			elif "Ambient outdoor" == tempS[0]:
				tempAmb.set(temp_to_format_str(float(tmpValue)))
	
root = Tk()
root.title("Sensors")
root.attributes('-fullscreen', True)

mainframe = ttk.Frame(root, padding="3 3 12 12")
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

ttk.Label(mainframe, text="Temperature", font=("Arial", 16)).grid(column=1, row=1, sticky=(N,E,W,S), columnspan=2)

ttk.Label(mainframe, text="Outside", font=("Arial", 16)).grid(column=1, row=2, sticky=S)
ttk.Label(mainframe, text="Water", font=("Arial", 16)).grid(column=2, row=2, sticky=S)
ttk.Label(mainframe, text="Pump Intake", font=("Arial", 16)).grid(column=1, row=4, sticky=S)
ttk.Label(mainframe, text="Cottage", font=("Arial", 16)).grid(column=2, row=4, sticky=S)
ttk.Label(mainframe, text="Outside (Lake)", font=("Arial", 16)).grid(column=1, row=6, sticky=S)
ttk.Label(mainframe, text="Cottage 2", font=("Arial", 16)).grid(column=2, row=6, sticky=S)

ttk.Label(mainframe, text="Humidity (inside)", font=("Arial", 16)).grid(column=1, row=8, sticky=(N,E,W,S), columnspan=2)

ttk.Label(mainframe, textvariable=tempOutside, font=("Arial", 16)).grid(column=1, row=3, sticky=S)
ttk.Label(mainframe, textvariable=tempLake, font=("Arial", 16)).grid(column=2, row=3, sticky=S)
ttk.Label(mainframe, textvariable=tempIntake, font=("Arial", 16)).grid(column=1, row=5, sticky=S)
ttk.Label(mainframe, textvariable=tempCottage, font=("Arial", 16)).grid(column=2, row=5, sticky=S)
ttk.Label(mainframe, textvariable=tempAmb, font=("Arial", 16)).grid(column=1, row=7, sticky=S)
ttk.Label(mainframe, textvariable=tempCottage2, font=("Arial", 16)).grid(column=2, row=7, sticky=S)
ttk.Label(mainframe, textvariable=humid, font=("Arial", 16)).grid(column=1, row=9, sticky=S)

# Update button
ttk.Button(mainframe, text="Force update", command=forceUpdate).grid(column=1, row=10, stick=(W,S), columnspan=2)

# Quit button
ttk.Button(mainframe, text="Quit", command=root.destroy).grid(column=1, row=11, stick=(W,S))

# Fahrenheit checkbox
global faren
faren = False
ttk.Checkbutton(mainframe, text='Fahrenheit', command=toggleF).grid(column=2, row=11, stick=(E,S))

# Add padding to all items
for child in mainframe.winfo_children(): child.grid_configure(padx=5, pady=5)

# Create job scheduler
global scheduler
scheduler = BackgroundScheduler()

# Set job to run once immediately
scheduler.add_job(update_temp)
# Schedule every 5 minute execution
scheduler.add_job(update_temp, 'interval', coalesce=True, minutes=5)

# Start scheduler
scheduler.start()

try:
	# Execute main GUI loop
	root.mainloop()
except (KeyboardInterrupt, SystemExit):
	scheduler.shutdown(False)