from tkinter import *
from tkinter import ttk
import sched, time
from datetime import datetime
import os
from apscheduler.schedulers.background import BackgroundScheduler
import requests

def toggleF():
	global faren
	global scheduler
	faren = not(faren)
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
	
	# Get info from pumphouse
	r = requests.get("http://10.10.2.20:1000/temp")
	
	# Throw error if applicable
	r.raise_for_status()
	
	# Split response
	threeTemp = r.text.rstrip("\r\n").split("|")
	
	# Set cottage temp close to time
	tempCottage.set( temp_to_format_str(float(cot)/1000) )
	
	for temp in threeTemp:
		tempS = temp.split(":")
		if "Lake Water" == tempS[0]:
			tempLake.set(temp_to_format_str(float(tempS[1])))
		elif "Outside Air" == tempS[0]:
			tempOutside.set(temp_to_format_str(float(tempS[1])))
		elif "Intake Pipe" == tempS[0]:
			tempIntake.set(temp_to_format_str(float(tempS[1])))
	
root = Tk()
root.title("Temperature")
root.attributes('-fullscreen', True)

mainframe = ttk.Frame(root, padding="3 3 12 12")
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
mainframe.columnconfigure(0, weight=1)
mainframe.rowconfigure(0, weight=1)

tempOutside = StringVar()
tempCottage = StringVar()
tempLake = StringVar()
tempIntake = StringVar()

ttk.Label(mainframe, text="Temperature", font=("Arial", 16)).grid(column=1, row=1, sticky=(N,E,W,S), columnspan=2)

ttk.Label(mainframe, text="Outside", font=("Arial", 16)).grid(column=1, row=2, sticky=S)
ttk.Label(mainframe, text="Lake", font=("Arial", 16)).grid(column=2, row=2, sticky=S)
ttk.Label(mainframe, text="Pump Intake", font=("Arial", 16)).grid(column=1, row=4, sticky=S)
ttk.Label(mainframe, text="Cottage", font=("Arial", 16)).grid(column=2, row=4, sticky=S)

ttk.Label(mainframe, textvariable=tempOutside, font=("Arial", 16)).grid(column=1, row=3, sticky=S)
ttk.Label(mainframe, textvariable=tempLake, font=("Arial", 16)).grid(column=2, row=3, sticky=S)
ttk.Label(mainframe, textvariable=tempIntake, font=("Arial", 16)).grid(column=1, row=5, sticky=S)
ttk.Label(mainframe, textvariable=tempCottage, font=("Arial", 16)).grid(column=2, row=5, sticky=S)

# Quit button
ttk.Button(mainframe, text="Quit", command=root.destroy).grid(column=1, row=6, stick=(W,S))

# Fahrenheit checkbox
global faren
faren = False
ttk.Checkbutton(mainframe, text='Fahrenheit', command=toggleF).grid(column=2, row=6, stick=(E,S))

# Add padding to all items
for child in mainframe.winfo_children(): child.grid_configure(padx=5, pady=5)

# Create job scheduler
global scheduler
scheduler = BackgroundScheduler()

# Set job to run once immediately
scheduler.add_job(update_temp)
# Schedule ever 5 minute executions
scheduler.add_job(update_temp, 'interval', coalesce=True, minutes=5)

# Start scheduler
scheduler.start()

try:
	# Execute main GUI loop
	root.mainloop()
except (KeyboardInterrupt, SystemExit):
	scheduler.shutdown(False)