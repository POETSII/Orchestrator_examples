# Netstress
A simple application to stress the POETS network.

The GraphType section of the XML defines two device types:
* A `receiver` that has an input pin to receive messages from "spammer" devices 
and an output pin to send status updates to the Supervisor once per second.
* A `spammer` that tries to constantly send messages to the `receiver`. The 
spammer will send constantly for approximately five seconds before going quiet
for approximately two seconds. This is to give the `receiver` a chance to send 
updates to the supervisor as the softswitch prioritises receiving messages and 
this blocks the sending of updates.

A single `spammer` device instance is able to block its own transmit link and
suppress all transmit activity on the `receiver`.

## Execution

This example should be executed as a single-device-per-thread.
The below snippet can be used as a callfile, replacing `<PATH_TO_ORCHESTRATOR>` 
with the path to your Orchestrator directory. For examples with lots of dummies
that use more than four boards, replace `topology /set1` with a load of a 
hardware description file for a full box, 
e.g. `topology /load = "/local/orchestrator-common/byron-solo.uif"`

```
task /path = "<PATH_TO_ORCHESTRATOR>/application_staging/xml"
task /load = "netstress_1board.xml"
topology /set1
topology /constrain="DevicesPerThread",1
task /build = "netstress_1board"
task /deploy = "netstress_1board"
task /init = "netstress_1board"
task /run = "netstress_1board"
```

## Examples
Five XML files are included to allow different links to be tested. Dummy, 
disconnected devices are used as padding for the default bucket-filling 
placement on a single-box system.
* `netstress_1mailbox.xml` has two device instances (one `receiver` and one 
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
Messages between the `spammer` & `receiver` and between the `receiver` and the
`supervisor` have to transition across the entire network. This will not work
with `topology /set1` and requires a hardware description file.

