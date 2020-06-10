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
is written, the default ADC datasheet values are used. The *zpub* program takes an 
environment variable `ADCCFG` that can be used to set this register before
starting ADC sample streaming.  e.g., for the DDC232 ADC (32 channel, 20bit) chip:

```shell
ADCCFG=0xf80 ./zpub      # 350pC full-scale range
ADCCFG=0xd80 ./zpub      # 300pC full-scale range
ADCCFG=0xb80 ./zpub      # 250pC full-scale range
ADCCFG=0x980 ./zpub      # 200pC full-scale range
ADCCFG=0x780 ./zpub      # 150pC full-scale range
ADCCFG=0x580 ./zpub      # 100pC full-scale range
ADCCFG=0x380 ./zpub      # 50pC full-scale range
ADCCFG=0x180 ./zpub      # 12.5pC full-scale range
```

Or, directly to the FPGA register, in C:

```c
/* Setting should be 0 - 7, for 12.5, 50, 100, 150, 200, 250, 300, and 350pC, respectively */
void ddc232_range(int setting) { 
        int fd = open("/dev/mem", O_RDWR|O_SYNC);
        uint8_t *mm = mmap(NULL, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x50000000);

        assert (setting >= 0 && setting < 8);
        assert (fd != -1 && mm != MAP_FAILED);
        *(uint32_t *)(mm + 0x401c) = 0x180 | (setting<<9);
        munmap(mm, 0x100000);
        close(fd);
}
```
*zpub.c* also takes an environment variable `ADCRATE` which represents the sample rate in integer
hz.  This is sent to the 32-bit FPGA register at address 0x50004020 as the number of 99Mhz periods
to wait inbetween samples.  A Sample C manipulation would be:

```c
void adc_rate(int hz) { 
        int fd = open("/dev/mem", O_RDWR|O_SYNC);
        uint8_t *mm = mmap(NULL, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x50000000);

        assert (hz > 0 && hz <= 100000);
        assert (fd != -1 && mm != MAP_FAILED);
        *(uint32_t *)(mm + 0x4020) = 99000000 / hz;
        munmap(mm, 0x100000);
        close(fd);
}
```

For high bandwidth FPGA to CPU data acquisition pipes, it may be necessary to
set realtime priorities on the zpub process (or low/idle priorities on
everything else) if the CPU is intended to be run overloaded.


