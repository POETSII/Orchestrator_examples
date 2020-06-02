What's This?
===

This directory holds a series of XML used to test different pieces of
functionality in the Mothership.

They are designed to be run using the Orchestrator batch system, with debugging
disabled. One such batch script could be (you'll need to change `<HOME>` and
`<TEST_NAME>`):

```
topology /load = "/local/orchestrator-common/1_box.uif"
task /path = "<HOME>/repos/orchestrator/application_staging/xml"
task /load = "<TEST_NAME>.xml"
link /link = "<TEST_NAME>"
task /build = "<TEST_NAME>"
task /deploy = "<TEST_NAME>"
task /init = "<TEST_NAME>"
task /run = "<TEST_NAME>"
```
