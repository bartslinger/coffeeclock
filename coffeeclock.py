from datetime import datetime
import time
import configparser
import requests
import os
import serial
import threading
from threading import Lock

serial_lock = Lock()

from apscheduler.schedulers.background import BackgroundScheduler

ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=0.1)
serial_thread_should_stop = False

armed_status = False
alarm_hours = 0
alarm_minutes = 0

connected_status = False

making_coffee = False

def enable_coffee():
    if connected_status == True:
        try:
            r = requests.get('http://192.168.114.147/gpio2/1', timeout=0.5)
            print('making coffee!')
        except:
            print('failed making coffee')
    else:
        print('coffeemachine not connected')


def disable_coffee():
    if connected_status == True:
        try:
            r = requests.get('http://192.168.114.147/gpio2/0', timeout=0.5)
            print('finished making coffee')
        except:
            print('failed quitting coffee')
    else:
        print('coffeemachine not connected')


def read_stored_data():
    global alarm_hours
    global alarm_minutes
    config = configparser.ConfigParser()
    config.read('coffeeclock.ini')
    alarm_hours = int(config['alarm']['hours'])
    alarm_minutes = int(config['alarm']['minutes'])

def save_data():
    global alarm_hours
    global alarm_minutes
    config = configparser.ConfigParser()
    config.read('coffeeclock.ini')
    config['alarm']['hours'] = str(alarm_hours)
    config['alarm']['minutes'] = str(alarm_minutes)
    with open('coffeeclock.ini', 'w') as configfile:
        config.write(configfile)

def tick():
    global making_coffee
    global armed_status
    #print('Tick! The time is: %s' % datetime.now())
    now = datetime.now()
    hours = now.hour
    minutes = now.minute
    seconds = now.second
    buf = (b'TIME=%02d:%02d\r\n' % (hours, minutes))
    serial_lock.acquire()
    ser.write(buf)
    serial_lock.release()
    # check for alarm
    if (hours == alarm_hours and minutes == alarm_minutes and connected_status == True and armed_status == True):
        if (making_coffee == False):
            making_coffee = True
            print('KOFFIETIJD --- KOFFIETIJD --- KOFFIETIJD --- KOFFIETIJD')
            #enable_coffee()
            armed_status = False
            send_armed_status()

def send_connected_status():
    flag = 1 if connected_status == True else 0
    buf = (b'CONNECTED=%d\r\n' % flag)
    serial_lock.acquire()
    ser.write(buf)
    serial_lock.release()    

def send_alarm_time():
    buf = (b'ALARM=%02d:%02d\r\n' % (alarm_hours, alarm_minutes))
    serial_lock.acquire()
    ser.write(buf)
    serial_lock.release()
    
def send_armed_status():
    flag = 1 if armed_status == True else 0
    serial_lock.acquire()
    ser.write(b'ARMED=%d\r\n' % flag)
    serial_lock.release()

def serial_read():
    global armed_status
    global alarm_hours
    global alarm_minutes
    while not serial_thread_should_stop:
        if ser.in_waiting > 0:
            reading = ser.readline().decode()
            #print(reading[0:6])
            if (reading == 'ARM\r\n'):
                if armed_status == True:
                    armed_status = False
                    #disable_coffee()
                else:
                    armed_status = True
                    #enable_coffee()
                send_armed_status()
            elif (reading == 'ALARM?\r\n'):
                send_alarm_time()
                send_armed_status()
                send_connected_status()
            elif (reading[0:10] == 'ALARM_SET=' and reading[12] == ':'):
                #print(reading)
                hours = int(reading[10:12])
                minutes = int(reading[13:15])
                alarm_hours = hours
                alarm_minutes = minutes
                save_data()
                

if __name__ == '__main__':
    read_stored_data()

    scheduler = BackgroundScheduler()
    scheduler.add_job(tick, 'cron', second='*')
    scheduler.start()
    print('Press Ctrl+{0} to exit'.format('Break' if os.name == 'nt' else 'C'))

    # Serial reading thread
    ser_thread = threading.Thread(target=serial_read)
    ser_thread.start()

    try:
        # This is here to simulate application activity (which keeps the main thread alive).
        while True:
            time.sleep(2)
            if armed_status:
                print("Alarm set for %02d:%02d" % (alarm_hours, alarm_minutes))

            try:
                r = requests.get('http://192.168.114.147/', timeout=0.5)
                if connected_status == False:
                    connected_status = True
                    send_connected_status()
            except:
                if connected_status == True:
                    connected_status = False
                    send_connected_status()

            #connected_status = True if connected_status == False else False
            #send_connected_status()
    except (KeyboardInterrupt, SystemExit):
        # Not strictly necessary if daemonic mode is enabled but should be done if possible
        scheduler.shutdown()
        serial_thread_should_stop = True

	
