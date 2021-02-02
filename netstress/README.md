# Netstress
A simple application to stress the POETS network.

The GraphType section of the XML defines two device types:
* A `receiver` that has an input pin to receive packets from "spammer" devices 
and an implicit output pin to send status updates to the Supervisor once per 
second.
* A `spammer` that tries to constantly send packets to the `receiver`. The 
spammer will send constantly for approximately five seconds before going quiet
for approximately two seconds. This is to give the `receiver` a chance to send 
updates to the supervisor as the default Softswitch configuration prioritises 
receiving packets: the `receiver` may be blocked from sending updates if the 
`spammer` is doing a good job.

In testing on Tinsel with the non-buffering Softswitch, a single `spammer` 
device instance is able to block its own transmit link and suppress all transmit 
activity on the `receiver`.

This example makes use of Tinsel-specific functionality to achieve timed delays.

In an ideal world, this example would comprise of a single type file shared 
between multiple graph instances. However, Orchestrator issue 170 prevents this.

## Output
This example writes output to `spammer_out.csv` in the execution directory. This
file contains details about how much spam was received, the peak spammer block
count and the minimum number of RTS buffer slots left on the spammer since the
last receiver-to-Supervisor packet was sent.

Instrumentation data should be interrogated along with this output to gain a
full understanding of the system's behaviour.

## Execution

This example should be executed as a single-device-per-thread.
The below snippet can be used as a callfile, replacing `<PATH_TO_ORCHESTRATOR>` 
with the path to your Orchestrator directory.

```
load /app = +"netstress.xml"
tlink /app = *
place /constraint = "MaxDevicesPerThread", 1
place /bucket = *
compose /app = *
deploy /app = *
initialise /app = *
run /app = 
```

## Examples
Five XML files are included  to allow different links to be tested. Dummy, 
disconnected devices are used as padding in all XMLs except for `netstress.xml`
to make the default bucket-filling placement put devices in specific places. The
same effect should be achievable with placement constraints.
* `netstress.xml` has two device instances (one `receiver` and one 
`spammer`) that will be placed on the same mailbox.
* `netstress_1board.xml` has 63 dummy `spammer` devices with no connections.
This forces the `receiver` onto the next board.
* `netstress_2board.xml` has 255 dummy `spammer` devices with no connections.
This forces the `receiver` onto the third board.
* `netstress_5board.xml` has 5119 dummy `spammer` devices with no connections.
This forces the `receiver` onto the sixth board. This will not work
with `topology /set1` and requires a hardware description file.
* `netstress_1box.xml` has 6111 dummy `spammer` devices with no connections.
This forces the `receiver` onto the final core-pair of the sixth board.
packets between the `spammer` & `receiver` and between the `receiver` and the
`supervisor` have to transition across the entire network. This will not work
with `topology /set1` and requires a hardware description file.

