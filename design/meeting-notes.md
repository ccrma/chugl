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
  - see: https://github.com/ccrma/chuck/compare/2023-chugl-int



## 9-12-2023

### Operator Overloading

**How did we get here?**:

Trying to figure out how to architect the GGen chuck class, and how it would then be extended by chugl.chug proved to be very difficult. It didn't really fit with the scenegraph architecture and rendering system. So instead, we had a fundamental change in thinking from "what graphics do we need to add to chuck core" to how can chuck language features and DLL API interface grow in order to make ChuGL work the way we want. This has many benefits
- no bending over backwards to split the graphics API line across chuck core and chugl.chug
- additions to chuck core and API interface can be shared across all chugins

Sample code: looks at .ck files in design/ directory 

### Next Steps

Side quests
1. cmake-ify chugl.chug and get it to build x-platform
2. specular lighting system

Main quest: learn WebGPU and begin reimplementing the chugl renderer
