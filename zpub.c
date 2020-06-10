#include <czmq.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>

/* This is optimized for i.MX6ul burst memory copies from EIM bus. */
extern void xmemcpy(void *dst, void *src, size_t n);

int main(int argc, char **argv) {
	const char *endpoint = getenv("ZPUB_ENDPOINT");
	const char *adcrate = getenv("ADCRATE"); /* Sample frequency in HZ */
	const char *adccfg= getenv("ADCCFG"); /* 16-bit config register sent to ADC itself */
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
	uint8_t *mm = mmap(NULL, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x50000000);
	uint32_t cur, last, sz;

	if (adccfg) *(uint32_t *)(mm + 0x401c) = strtoul(adccfg, NULL, 0);
	if (adcrate) *(uint32_t *)(mm + 0x4020) = 99000000 / strtoul(adcrate, NULL, 0);

	if (endpoint) { 
	        zsock_t *sk = zsock_new_pub(endpoint);
		assert(sk != NULL);
		last = cur = *(uint16_t *)(mm + 0x404c);
		while (1) {
			zframe_t *zf;
			uint8_t *d;
		       
			cur = *(uint16_t *)(mm + 0x404c);
			if (last > cur) sz = (cur + 0x10000) - last;
			else if (last == cur) {
				usleep(1000);
				continue;
			} else sz = cur - last;

			zf = zframe_new(NULL, sz);
			if (!zf) continue;
			d = zframe_data(zf);
			xmemcpy(d, mm + 0x8000 + last, sz);
			
			zframe_send(&zf, sk, 0);
			last = cur;
		}
	} else return 1;
}
