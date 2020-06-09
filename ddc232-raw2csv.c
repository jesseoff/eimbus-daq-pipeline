#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

/* Takes raw binary FPGA data (32 chans of 20-bit samples) on stdin and 
 * outputs CSV on stdout. Optional command line argument is a floating 
 * point sample rate. e.g. 
 *
 *   ddc232-raw2csv 333.333
 *
 * Which outputs CSV samples at 333.333Hz sample rate.  
 */

static double rate = 6000, acc = 0;

/* ADC sends channel 31 first, channel 0 last */
static inline void sample(uint32_t v) {
  int i;
  static int n = 0;
  static uint32_t chans[32];
  chans[31 - n] = v;
  if (n == 31) {
    n = 0;
    acc += rate;
    if (acc >= 6000) acc -= 6000; else return;
    for (i = 0; i < 31; i++) printf("%d,",chans[i]);
    printf("%d\n", chans[31]);
  } else n++;
}

/* ADC sends sample bits from from most to least significant */
static inline void out4(int n) { 
  static uint32_t x;
  static int seq = 0;
  x = (x<<4)|n;
  if (seq == 4) {
    sample(x);
    x = seq = 0;
  } else seq++;
}

static inline void out8(int b) {
  out4(b >> 4);
  out4(b & 0xf);
}

int main(int argc, char **argv) {
  int hw;
  if (argc >= 2) {
    rate = strtod(argv[1], NULL);
    assert(rate <= 6000 && rate > 0);
  } 
  while ((hw = getchar()) != EOF) { /* FIFO is 16-bit, little-endian */
    out8(getchar());
    out8(hw);
  }
}
