# Plate_heat Orchestrator Example

A POETS implementation of a simple problem - a canonical heated plate with corners
pinned at extreme temperatures implemented in 32-bit floating point.

At a high level, the problem is attacked as a grid of points with parallelisation
at the grid-point level, with each point represented by an individual POETS device.
Each device (simply) averages the temperature of its connected neighbours and emits
a packet containing its latest temperature.

Final temperature and per-device instrumentation are transmitted to the Supervisor
by each device when the device consideres that it has finished. How the device 
determines whether it is complete is an issue in and of itself.


**__N.B.__**: XML size can become significant when you go above 1000x1000.

**__N.B.__**: Do not try to execute *plate_heat_type.xml* with the Orchestrator - 
it is not a complete application and will fail.

This example includes multiple slightly different approaches to the heated plate 
problem and termination detection. Three different generators are included:


### `gentest`/Packet storm with Heartbeats

This example uses Heartbeats for experiment termination. Each device has a 
dedicated input pin to receive Hearbeat packets. Each device keeps track of 
a local counter of how many Heartbeat packets it has received since the last 
update it received from its neighbours. When the counter hits a defined limit,
the device considers that it is done and exfiltrates its data to the Supervisor.
The counter is reset any time an update is received.

If an update is received after the device has indicated that it has completed, 
the device sends another packet to the Supervisor cancelling its completion.

Heartbeats manifest as a standard packet so can be generated anywhere within the 
fabric or by the Supervisor. This example (ab)uses knowledge of the Softswitch 
to avoid sending any actual packets across the fabric: the first device hosted 
by a thread generates a Heartbeat packet any time its `OnIdle` handler is called
and then passes this directly to the `OnReceive` handler of each other device 
hosted by the thread. The use of `OnIdle` to generate Heartbeats means that they 
are only generated when a thread is less busy, reducing any effect the processing 
overhead has on computation.

Two type files are included for this example:
  * `plate_heat_type.xml`: An unrestricted packet and prcoessing storm. The local
    temperature is recalculated as part of the receive handler for each update
    packet.
    
  * `plate_heat_type_buf.xml`: A moderated packet and processing storm. The local
    temperature is only recalculated when `OnIdle` is triggered. This potentially 
    reduces the amount of computation per emitted packet as updates from multiple 
    neighbours can be received between each recalculation.

The local temperature is only updated, and a packet emitted, after a recalculation
if the change in temperature is above a defined threshold. Changes in temperature
below the defined threshold are essentially ignored.


### `gentestalt`/Packet storm without Heartbeats

This example is similar to the moderated packet storm, except that Heartbeats are 
replaced with device-local counters that are incremented directly in the `OnIdle`
handler and reset any time a packet is received from a neighbour. When the counter 
reaches a defined limit, the device indicates to the Supervisor that it is done.

A single type file is included for this example: `plate_heat_type_alt.xml`


### `gentestgals`/GALS approah to the problem

Each device keeps track of the index of the oteration/step that is is on and will 
only send an update for a step `N` when it has received an update from all of its 
neighbours for step `N - 1`. Each device remains within one step of its neighbours, 
but different areas of the problem are able to progress at different rates.

Completion detection in this example is achieved by counting the number of steps 
that have occured since the last significant change in local temperature. Local
changes below a defined threshold are ignored.

A single type file is included for this example: `plate_heat_type_gals.xml`

## Examples

Pre-generated examples for some smaller plates exist in the *pregen* directory. 
Larger plates can be generated with *gentest*. The contents of the *pregen*
directory were generated with `gentestalt`.


## Getting Started

The examples and their call files in the *pregen* folder can be used as-is to 
test orchestrator functionality.

## Gentest

This repository includes three different generators to generate plates of arbitary
sizes. Each generator takes in a pre-provided type file (a definition of the graph 
type, which defines application functionality) and outputs everything needed to make
a full POETS application.

Each flavour of Gentest accepts a number of command line arguments that affect XML 
generation. The available options can be listed by passing `--help` to the generator.
Current arguments are:

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
      -g[filename], --type=[filename]   Filename of the Type file,
                                        default=plate_heat_type.xml
      -o[string]                        String to append to the generated
                                        filename before .xml, default=""
      -O[string]                        String to use instead of the generated
                                        filename, default=""
      -s[int]                           Set whether devices are generated
                                        linearly (0) or in square blocks (1 =
                                        thread-level, 2 = box-level), default
                                        = 0
      -u[int], --sd=[int]               Square Dimension - used to set the x &
                                        y dimension of the square size,
                                        default = 32 (e.g. 1024 devices)
      --ThreadMaxCount=[int]            Set the maximum number of threads
                                        (used for Supervisor Instrumentation
                                        Array sizing). Default = 49152
      -i[int]                           Set the OnIdle count required to
                                        trigger a HB, default >=1000 (scales
                                        with problem size)
      -z[int], --hb=[int]               Set the HB count required to trigger a
                                        finish message, default = 10

### Pre-requisites

* A C++ compiler that supports at least c++11
* make (optional)
* Python 3 (optional, for Visualiser) with:
 * argparse
 * numpy
 * matplotlib

### Compatibility

gentest has been tested on Windows 8.1, Windows 10, WSL, Ubuntu, Debian and MacOS 
- it should work on anything with a c++11-compatible compiler. The makefile uses c++17.

### Building Gentest

     make

This builds all three generators. Specific generators can be built individually, e.g. 
`make gentestalt` only makes `gentestalt`. `make gentestalt gentestgals`makes both 
`gentestalt` and `gentestgals`.

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
be called in the same directory as `plate_NxN_out.csv`, which is usually the
Orchestrator's `bin` directory.

Four parameters are visualised per-device:

* The final temperature
* The number of finished messages sent
* The number of update messages received
* The number of output messages sent. N.B. this is the number of times the pin's
OnSend handler has been called, so actual emitted packet count may be 4x larger.
