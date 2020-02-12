The FPGA continuously writes analog data to a 64kbyte circular buffer inside the
Cyclone IV FPGA. This buffer starts at address 0x50008000, physical. The 16-bit
register at 0x5000404c represents the next address (shifted right 1 bit) to be
written into the hardware buffer by the FPGA. Software must poll this register
and consume ADC data as it is being captured. A program *zpub* performs the
appropriate polling and copying into Linux userspace by accessing the hardware
directly from the /dev/mem Linux driver. This program sets up a ZeroMQ publisher
socket, taken from the environment variable `ZPUB_ENDPOINT`, and passes data
continuously to it. The `ZPUB_ENDPOINT` can be set to, e.g. `tcp://*:1234` to
send ADC data via the ethernet to a remote computer, or something like
`ipc:///tmp/fpga` if a program local to the imx6 is to consume the samples
instead.

A sample program *zsub* is also included. This program takes an environment
variable `ZSUB_ENDPOINT`, connects a ZeroMQ subscriber socket to it, and sends
all output to stdout. Both *zpub* and *zsub* must be compiled with the -lczmq
switch, to allow linking to the CZMQ C language ZeroMQ bindings. `zpub.c` also
needs `memcpy_arm.S` since Linux's default memcpy() implementation uses NEON
registers which is extremely slow in copying data from the imx6 EIM bus to
SDRAM. `memcpy_arm.S` comes originally from the NetBSD project, which is under the BSD
license, but also has been further optimized by me.

The samples acquired by the FPGA are the raw digital samples from the ADC,
captured in order from lowest channel to highest channel. 20-bit samples are
combined and packed together in the tightest configuration, i.e. 32 20-bit
samples are sent in 80 byte chunks. Guarantees from the FPGA design and ZeroMQ
insure there will never be partial blocks of samples in the output stream.

The 32-bit register at 0x5000401c is the ADC configuration register. Changing
the value here results in resetting the ADC and sending a new configuration
register as documented in the particular ADC's data sheet.  Until/Unless this register
is written, the default ADC datasheet values are used and the sample rate is at maximum.

For high bandwidth FPGA to CPU data acquisition pipes, it may be necessary to
set realtime priorities on the zpub process (or low/idle priorities on
everything else) if the CPU is intended to be run overloaded.

