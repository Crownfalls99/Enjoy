# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <unistd.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <pthread.h>

# define PORT_BASE 5000
# define NODE_NUM 3
# define BUFSIZE 64

int port;

void errCast(char* msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}


void Pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {

	int res = pthread_create(thread, attr, start_routine, arg);
	if (res != 0 ) errCast("pthread create");
}

void* Listener(void* arg) {
	int lsd = *( (int*) arg );

	struct sockaddr_in listening;
	
	socklen_t len = sizeof(listening);
	int listening_sd = accept(lsd, (struct sockaddr*)&listening, &len);
	if (listening_sd == -1) errCast("accept");
	
	printf("port %d connected\n", PORT_BASE + port);

	
	char buf[BUFSIZE];
	recv(listening_sd, buf, sizeof(buf), 0);

	buf[BUFSIZE - 1] = '\0';
	printf("port %d received %s\n", port, buf);


	return NULL;
}

void* Sender(void* arg) {
	printf("port %d sender start\n", port);

	int ssd = socket(AF_INET, SOCK_STREAM, 0);
	if (ssd == -1) errCast("socket create");


	struct sockaddr_in sending;
	sending.sin_family = AF_INET;
	sending.sin_port = htons(PORT_BASE + ((port + 1) % NODE_NUM));

	struct in_addr ip;
	memset(&ip, 0, sizeof(ip));
	int res1 = inet_pton(AF_INET, "127.0.0.1", &ip);
	if (res1 == -1) errCast("inet_pton");

	sending.sin_addr = ip;
	memset(&sending.sin_zero, 0, sizeof(sending.sin_zero));

	while (connect(ssd, (struct sockaddr*)&sending, sizeof(sending)) != -1) ;
	
	char* msg = "fuck";

	send(ssd, msg, strlen(msg), 0);

	printf("port %d sender ends\n", port);
	close(ssd);

	return NULL;
}

int main(int argc, char* argv[]) {
	if (argc != 2) errCast("invalid argument");

	int lsd = socket(AF_INET, SOCK_STREAM, 0);
	if (lsd == -1) errCast("socket create");

	int optval = 1;
	
	int res1 = setsockopt(lsd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res1 == -1) errCast("setsockopt");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	
	port = atoi(argv[1]);
	addr.sin_port = htons(PORT_BASE + port);

	ssize_t res2 = bind(lsd, (struct sockaddr*)&addr, sizeof(addr));
	if (res2 == -1) errCast("bind");

	int res3 = listen(lsd, 5);
	if (res3 == -1) errCast("listen");

	pthread_t listener;
	Pthread_create(&listener, NULL, Listener, (void*)&lsd);
		
	pthread_t sender;
	Pthread_create(&sender, NULL, Sender, NULL);

	pthread_join(sender, NULL);
	pthread_join(listener, NULL);

	close(lsd);
	printf("port %d listener ends\n", port);
	return 0;
}

