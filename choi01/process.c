# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <unistd.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <pthread.h>

# define BUFSIZE 4
# define TERMINATE 999

int base, n_nodes, port;
int tick;

void err_cast(char* msg) {
	printf("%s\n", msg);
	exit(1);
}

void int2str(char* buf, int n) {
	memset(buf, 0, BUFSIZE);
	sprintf(buf, "%d", n);
}

void* sender_routine(void* arg) {
	int res;
	int ssd = socket(AF_INET, SOCK_STREAM, 0);
	if (ssd == -1) err_cast("socket");

	struct in_addr ip;
	memset(&ip, 0, sizeof(ip));
	res = inet_pton(AF_INET, "127.0.0.1", &ip);
	if (res == -1) err_cast("inet_pton");

	struct sockaddr_in sending;
	sending.sin_family = AF_INET;
	sending.sin_port = htons(base + ((port + 1) % n_nodes));
	sending.sin_addr = ip;
	memset(&sending.sin_zero, 0, sizeof(sending.sin_zero));

	sleep(tick);
	res = connect(ssd, (struct sockaddr*)&sending, sizeof(sending)) ;
	if (res == -1) err_cast("connect");
	
	char buf[BUFSIZE];
	int2str(buf, port);
	
	res = (int)send(ssd, buf, strlen(buf), 0);
	if (res == -1 || res == 0) err_cast("send");

	


	/*
	for (int i = 10; i < 13; i++) {
		sleep(tick);
		int2str(buf, i);
		res = (int)send(ssd, buf, strlen(buf), 0);
		if (res == -1 || res == 0 ) err_cast("send");
	}
	*/

	sleep(tick);
	int2str(buf, TERMINATE);
	res = (int)send(ssd, buf, strlen(buf), 0);
	if (res == -1 || res == 0) err_cast("send");

	close(ssd);
	return NULL;
}

int main(int argc, char* argv[]) {
	if (argc != 4) err_cast("invalid argument");
	base = atoi(argv[1]);
	n_nodes = atoi(argv[2]);
	port = atoi(argv[3]);
	tick = (int)(0.5 * n_nodes + 0.5);

	int res;
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) err_cast("socket");

	int optval = 1;
	res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) err_cast("setsockopt");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(base + port);

	res = (int)bind(sd, (struct sockaddr*)&addr, sizeof(addr));
	if (res == -1) err_cast("bind");

	res = listen(sd, 5);
	if (res == -1) err_cast("listen");

	pthread_t sender;
	res = pthread_create(&sender, NULL, sender_routine, NULL);
	if (res != 0 ) err_cast("pthread_creat");

	struct sockaddr_in listening;
	socklen_t len = sizeof(listening);
	int lsd = accept(sd, (struct sockaddr*)&listening, &len);
	if (lsd == -1) err_cast("accept");

	char buf[BUFSIZE];

	while(1) {
		res = recv(lsd, buf, sizeof(buf), 0);
		if (res == -1 || res == 0) err_cast("recv");
		
		if(atoi(buf) == TERMINATE) break;

		buf[res] = '\0';
		printf("node %d receive : %s\n", port, buf);
	}

	res = pthread_join(sender, NULL);
	if (res != 0) err_cast("pthread_join");

	close(sd);
	close(lsd);
	printf("node %d ended\n", port);
	return 0;
}

