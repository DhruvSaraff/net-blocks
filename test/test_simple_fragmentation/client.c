#include "nb_runtime.h"
#include <stdio.h>
#include <unistd.h>

#define bytes 100000
char buf[bytes];

char client_id[] = "1.0.0.2";
char server_id[] = "1.0.0.1";

static void callback(int event, nb__connection_t * c) {
}

int main(int argc, char* argv[]) {
	nb__ipc_init("/tmp/ipc_socket", 0);
	printf("IPC initialized\n");

	unsigned int server_id_i = inet_addr(server_id);
	unsigned int client_id_i = inet_addr(client_id);

	nb__my_host_id = client_id_i;

	nb__net_init();


	nb__connection_t * conn = nb__establish(server_id_i, 8080, 8081, callback);

	for (int i = 0; i < bytes; i++) {
		buf[i] = i % 128;
	}
	
	int sent = nb__send(conn, buf, bytes);
	printf("Sent %d bytes\n", sent);

	usleep(100 * 1000);

	nb__destablish(conn);
	return 0;
	
}
