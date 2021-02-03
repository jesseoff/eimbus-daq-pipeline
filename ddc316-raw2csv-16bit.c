#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

/* Takes raw binary FPGA data (32 chans of 16-bit samples) on stdin and 
 * outputs CSV on stdout. 
 */

/* ADC sends channel 0 first, channel 31 last */
static inline void sample(uint32_t v) {
  int i;
  static int n = 0;
  static uint32_t chans[32];
  chans[n] = v;
  if (n == 31) {
    n = 0;
    for (i = 0; i < 31; i++) printf("%d,",chans[i]);
    printf("%d\n", chans[31]);
  } else n++;
}

/* ADC sends sample bits from from most to least significant */
static inline void out4(int n) { 
  static uint32_t x;
  static int seq = 0;
  x = (x<<4)|n;
  if (seq == 3) {
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
  while ((hw = getchar()) != EOF) { /* FIFO is 16-bit, little-endian */
    out8(getchar());
    out8(hw);
  }
}
