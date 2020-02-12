#include <czmq.h>
#include <stdio.h>

int main(int argc, char **argv) {
	const char *endpoint = getenv("ZSUB_ENDPOINT");
	if (endpoint) { 
	        zsock_t *sk = zsock_new_sub(endpoint, "");
		while (1) {
			zframe_t *zf = zframe_recv(sk);
			if (!zf) continue;
			fwrite(zframe_data(zf), zframe_size(zf), 1, stdout);
			zframe_destroy(&zf);
		}
	} else return 1;
}
