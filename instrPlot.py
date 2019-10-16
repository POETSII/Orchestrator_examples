#!/usr/bin/env python 

import glob
import csv
import numpy 
import matplotlib
import matplotlib.pyplot as plt
from scipy import interpolate


size = 400
fpattern = 'cycles_*.csv'

totaly = numpy.zeros((size))
xnew = numpy.arange(0, size, 1)
threads = []


files = glob.glob(fpattern)
threadCount = len(files)

for fname in files:
    thread = []
    
    #original X vals
    thread.append(numpy.loadtxt(open(fname, "rb"), delimiter=",", skiprows=1, usecols=(4)))

    #original Y vals
    thread.append(numpy.loadtxt(open(fname, "rb"), delimiter=",", skiprows=1, usecols=(7)))

    #Interpolate
    fcy0 = interpolate.interp1d(thread[0],thread[1])

    #Save the output
    thread.append(fcy0(xnew))
    
    #Add to the total
    totaly += thread[2]
    
    #Append to the system data
    threads.append(thread)
    
totaly = totaly/100000


# Generic plotting setup
plt.rc("font", **{'size': 18})
matplotlib.rcParams['axes.linewidth'] = 1 # set the value globally


# First plot.
plt.figure(figsize=(18,10))

#Total messages plot
plt.plot(xnew, totaly, '-')
#plt.plot([307, 307], [0, 11], 'k--')
#plt.text(310, 6.5, "Completion at\n307 seconds", fontsize=18)
plt.title('Heated Plate, Alternative Softswitch', fontsize=25)
plt.xlabel('Wallclock time (s)', fontsize=20)
plt.ylabel(r'Message send rate ($10^6$/s)', fontsize=20)
plt.axis([0,size,0,11])
plt.grid(linestyle='-', linewidth='1', which='major')
plt.savefig('msgs.png')



# Rainbow plot
plt.figure(figsize=(18,10))

for i in range(threadCount):
    plt.plot(xnew, threads[i][2], '-', label='Thread '+str(i))

plt.axis([0,size,0,35000])
plt.title('Heated Plate, Alternative Softswitch', fontsize=25)
plt.ylabel('Message send rate', fontsize=20)
plt.xlabel('Wallclock time (s)', fontsize=20)
plt.grid(linestyle='-', linewidth='1', which='major')
plt.savefig('msgs_rainbow.png')


# Selected threads
plt.figure(figsize=(18,10))

plt.plot(xnew, threads[0][2], '-', label='Thread '+str(0))
if threadCount>10:
    plt.plot(xnew, threads[int(threadCount/4)][2], '-', label='Thread '+str(int(threadCount/4)))
    plt.plot(xnew, threads[int((threadCount/4)*3)][2], '-', label='Thread '+str(int((threadCount/4)*3)))

plt.plot(xnew, threads[threadCount-1][2], '-', label='Thread '+str(threadCount-1))


plt.title('Heated Plate, Alternative Softswitch', fontsize=25)
plt.ylabel('Message send rate', fontsize=20)
plt.xlabel('Wallclock time (s)', fontsize=20)
plt.grid(linestyle='-', linewidth='1', which='major')
plt.savefig('msgs_threads.png')

plt.show()






