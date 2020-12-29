#include "main.h"
#include "tuto.h"

int main(int argc, char* argv[])
{
	/*Ctrl+C catch*/
	if (sigaction_wrapper(SIGINT, sigint_handler) == -1)
		return EXIT_FAILURE;

	struct sockaddr_in servaddr;
	int fd = setup(&servaddr);

	/*Boucle de service*/
	char message[1024];
	struct sockaddr client;

	while (!quit)
	{
		memset(message, 0, 1024);
		socklen_t len = sizeof(client);
		recvfrom(fd, message, 1024, 0, (struct sockaddr*)&client, &len);
		MIRC_DGRAM dgram;
		parseDatagram(message, 1024, dgram);
	}

	return EXIT_SUCCESS;
}

int setup(struct sockaddr_in* addr)
{
	struct sockaddr_in servaddr;
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	/*Connexion à la socket*/
	int fd = NewSocket(AF_INET, SOCK_DGRAM, 0, (struct sockaddr*)&servaddr);
	if (fd < 0)
	{
		perror("Problème de création de connexion.");
		exit(2);
	}

	/*Coordonnées du serveur*/
	memset(host, 0, NI_MAXHOST);
	memset(service, 0, NI_NAMEREQD);
	if (getnameinfo((struct sockaddr*)&servaddr, sizeof(servaddr), host, NI_MAXHOST, service, NI_MAXSERV, NI_NAMEREQD))
		printf("Serveur lancé à l'adresse %s:%d\n", host, ntohs(servaddr.sin_port));
	else
		printf("Serveur local sur le port %d\n", ntohs(servaddr.sin_port));
	*addr = servaddr;
	return fd;
}

/*
 * wrapper for the sigaction function
 */
int sigaction_wrapper(int signum, handler_t* handler) {
	struct sigaction act;
	sigset_t set;

	act.sa_handler = handler;
	sigemptyset(&set);
	act.sa_mask = set;
	act.sa_flags = SA_RESTART;

	return sigaction(signum, &act, NULL);
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.
 */
void sigint_handler(int sig)
{

	cout << endl << "Exiting." << endl;

	quit = true;


	return;
}