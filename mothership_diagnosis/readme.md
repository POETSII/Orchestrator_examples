What's This?
===

This directory holds a series of XML used to test different pieces of
functionality in the Mothership.

They are designed to be run using the Orchestrator batch system, with debugging
disabled. One such batch script could be (you'll need to change `<TEST_NAME>`):

```
load /engine = "/local/orchestrator-common/1_box.uif"
load /app = +"<TEST_NAME>.xml"
tlink /app = *
place /rand = *
compose /app = *
deploy /app = *
initialise /app = *
run /app = *
```
