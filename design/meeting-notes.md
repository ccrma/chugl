# ChuGL Design Meeting Notes

## 9-5-2023

### Questions
- Spencer's update to Chugin build system: what does Chugl need to change?
- Fix inheritance bug in DLL query


### Remarks

- use linker /FORCE option to create chugl.dll executable even though the chuck event fns wait() and queue_event() are not exposed through the chugin API (used to wait and broadcast chugl events)
  - this is equivalent to what Spencer did in MAUI using -Wl -undefined,dynamic_lookup to delay resolving its Chuck_Event declarations until runtime

- can set visual studio breakpoints in the DLL! set the breakpoint in the DLL project and change the project property Debugging/command to run chuck
  - BUT this caused a segfault when actually executing

- To resolve the segfault, we added 2 functions to the DL VM API
  1. create_event_buffer()  -- returns a new CBufferSimple that offers chugins a thread-safe way to create and *trigger* custom events
  2. queue_event() -- places event on a queue for VM to process, 
- these two functions combined allow broadcasting events from threads other than the chuck audio thread
- In addition, the CK_DL_API instance is now passed as a member of the QUERY object

