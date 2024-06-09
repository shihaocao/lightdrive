# lightdrive
A task oriented music visualizer scheduler

## Background
I want to create an async task driven scheduler to help create light shows

# Dev Log
## 06/09 Start Project
I copied in my code somewhat from vvtol to bootstrap things. Going to first see if I can get
an anyio event loop to execute with sub ms resolution. I am making the big architectural gamble that says:
I have no requirement for the full event loop to be microcontroller only.