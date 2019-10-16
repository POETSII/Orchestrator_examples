#!/usr/bin/env python 

import csv
import numpy 
import matplotlib.pyplot as plt
import argparse


# Parse the user-supplied dimension.
parser = argparse.ArgumentParser(description='Heated plate plotter.')
parser.add_argument("-d", "--dimension", help="The dimension of the heated plate", required=True, type=int)
args = parser.parse_args()
dimension = args.dimension

# Open the data files.
try:
    a = numpy.loadtxt(open("plate_"+str(dimension)+"x"+str(dimension)+"_out.csv", "rb"), delimiter=",", skiprows=1, usecols=(0,1,2))
    fc = numpy.loadtxt(open("plate_"+str(dimension)+"x"+str(dimension)+"_out.csv", "rb"), delimiter=",", skiprows=1, usecols=(0,1,3))
    ic = numpy.loadtxt(open("plate_"+str(dimension)+"x"+str(dimension)+"_out.csv", "rb"), delimiter=",", skiprows=1, usecols=(0,1,5))
    oc = numpy.loadtxt(open("plate_"+str(dimension)+"x"+str(dimension)+"_out.csv", "rb"), delimiter=",", skiprows=1, usecols=(0,1,4))
except FileNotFoundError:
    print("Unable to find data file - does it exist?")
else:
    b = a[:,2]
    c = fc[:,2]
    d = ic[:,2]
    e = oc[:,2]

    b.shape = (b.size//dimension, dimension)
    c.shape = (c.size//dimension, dimension)
    d.shape = (b.size//dimension, dimension)
    e.shape = (c.size//dimension, dimension)

    plt.figure(1, figsize=(15,15))
    plt.suptitle('200x200 Heated plate')
    plt.subplot(221)
    plt.imshow(b, cmap='RdBu', interpolation='nearest', origin='lower')  #cmap='jet'
    plt.title('Temperature')
    #plt.imshow(g1, cmap='RdBu',  origin='lower', vmin=-500, vmax=500)  #cmap='jet' interpolation='nearest',
    #plt.title('Row Grad')
    plt.colorbar()

    plt.subplot(222)
    plt.imshow(c, interpolation='nearest', origin='lower')
    plt.title('Fincount')
    #plt.imshow(g2, cmap='RdBu',  origin='lower', vmin=-500, vmax=500)  #cmap='jet' interpolation='nearest',
    #plt.title('Col Grad')

    plt.colorbar()

    plt.subplot(223)
    plt.imshow(d, interpolation='nearest', origin='lower', )
    plt.title('InCount')
    plt.colorbar()

    plt.subplot(224)
    plt.imshow(e, interpolation='nearest', origin='lower')
    plt.title('OutCount')
    plt.colorbar()

    plt.savefig(str(dimension)+'x'+str(dimension)+'.png')
    plt.show()

