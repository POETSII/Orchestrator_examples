# Plate_heat Orchestrator Example

A POETS implementation of a simple problem - a heated plate.

This example uses Heartbeats for experiment termination and Supervisors for data exfiltration.

Pre-generated examples for some smaller plates exist in the *pregen* directory. Larger plates can be generated with *gentest*.

**__N.B.__**: XML size can become significant when you go above 1000x1000.

**__N.B.__**: Do not try to execute *plate_heat_type.xml* with the Orchestrator - it is not a complete application and will fail.

## Getting Started

The examples in the *pregen* folder can be used as-is to test orchestrator functionality.

## Gentest

Allows you to generate plates of arbitary sizes. Takes *plate_heat_type.xml* (a definition of the graph type, which defines application functionality) and adds everything needed to make a full POETS application.

### Pre-requisites

* A C++ compiler that supports at least c++11
* make (optional)

### Compatibility

gentest has been tested on Windows 8.1, Debian 9 and MacOS 10.14 - it should work on anything with a c++11-compatible compiler. The makefile uses c++17.

### Building Gentest

     make

### Generating Arbitrary Size Plates

     ./gentest -x[xSize] -y[ySize]

e.g. to generate a 50x50 plate, execute

     ./gentest -x50 -y50

### Changing/adding fixed nodes

Fixed nodes are defined in the *fNodes* vector (about line 86 of *gentest.cpp*). Additional nodes can be added to this vector.