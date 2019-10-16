# Plate_heat Orchestrator Example

A POETS implementation of a simple problem - a heated plate.

This example uses Heartbeats for experiment termination and Supervisors for data 
exfiltration. Final temperature and per-device instrumentation are piggy-backed
on the finished messages.

Pre-generated examples for some smaller plates exist in the *pregen* directory. 
Larger plates can be generated with *gentest*.

**__N.B.__**: XML size can become significant when you go above 1000x1000.

**__N.B.__**: Do not try to execute *plate_heat_type.xml* with the Orchestrator 
- it is not a complete application and will fail.

## Getting Started

The examples in the *pregen* folder can be used as-is to test orchestrator 
functionality.

## Gentest

Allows you to generate plates of arbitary sizes. Takes *plate_heat_type.xml* (a 
definition of the graph type, which defines application functionality) and adds 
everything needed to make a full POETS application.

Gentest accepts a number of command line arguments to configure the generated
XML:

     OPTIONS:
      
      -h, --help                        Display this help menu
      All of these arguments are
      optional:
    
      -x[int]                           Set X Dimension, default=3
      -y[int]                           Set Y Dimension, default=3
      -d[int]                           Set Both Dimensions, default=3
      -m[float]                         Set the minimum change in temperature
                                        required to trigger a message. Default
                                        = 0.5
      -t[float]                         Set the temperature of the fixed
                                        nodes. Default = 100
      -f[int]                           Fixed-node pattern. (0) 2 opposite
                                        corners, (1) 4 opposite corners.
                                        Default=0
      -x[filename], --type=[filename]   Filename of the Type file,
                                        default=plate_heat_type.xml
      -o[string]                        String to append to the generated
                                        filename before .xml, default=""
      -s[int]                           Set whether devices are generated
                                        linearly (0) or in blocks (1 =
                                        thread-level, 2 = box-level), default
                                        = 0
      --ThreadMaxCount=[int]            Set the maximum number of threads
                                        (used for Supervisor Instrumentation
                                        Array sizing). Default = 49152

### Pre-requisites

* A C++ compiler that supports at least c++11
* make (optional)
* Python 3 (optional, for Visualiser) with:
 * argparse
 * numpy
 * matplotlib

### Compatibility

gentest has been tested on Windows 8.1, Debian 9 and MacOS 10.14 - it should 
work on anything with a c++11-compatible compiler. The makefile uses c++17.

### Building Gentest

     make

### Generating Arbitrary Size Plates

     ./gentest -x[xSize] -y[ySize]
     
or

     ./gentest -d[xSize]

e.g. to generate a 50x50 plate, execute

     ./gentest -x50 -y50

### Changing/adding fixed nodes

Fixed nodes are defined in the *fNodes* vector (about line 86 of *gentest.cpp*).
Additional nodes can be added to this vector.

### Visualising Output

A crude python plotter (plot.py) is included to visualise the output. Invoke the
plotter with a "-d N" argument, where N is the x/y dimension. plot.py needs to 
be called in the same directory as "plate_NxN_out.csv".

Four parameters are visualised per-device:

* The final temperature
* The number of finished messages sent
* The number of update messages received
* The number of output messages sent. N.B. this is the number of times the pin's
OnSend handler has been called.