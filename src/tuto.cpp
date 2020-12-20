#include "tuto.h"

void serveur()
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
	memset(serv, 0, NI_NAMEREQD);
	if (getnameinfo((struct sockaddr*)&servaddr, sizeof(servaddr), host, NI_MAXHOST, service, NI_MAXSERV, NI_NAMEREQD))
		printf("* Server ECHO at : %s:%d\n", host, ntohs(servaddr.sin_port));
	else
		printf("* Server ECHO at : localhost:%d\n", ntohs(servaddr.sin_port));

	/*Boucle de service*/
	char message[1024];
	struct sockaddr client;

	while (true)
	{
		memset(message, 0, 1024);
		socklen_t len = sizeof(client);
		recvfrom(fd, message, 1024, 0, (struct sockaddr*)&client, &len);
		/*ECHO*/
		sendto(fd, message, strlen(message), 0, (struct sockaddr*)&client, len);
	}
}

void client(int argc, char* argv[])
{
	if (argc < 4) {
		perror("Manque <adresse> <port> <votre message>");
		exit(2);
	}
	struct sockaddr addr;

	/*Connexion*/
	int fd = NewSocket(AF_INET, SOCK_DGRAM, 0, (struct sockaddr*)&addr);
	if (fd == -1) {
		perror("Erreur connexion.");
		exit(2);
	}

	struct hostent* hs;
	struct sockaddr_in servaddr;
	socklen_t ls = sizeof(servaddr);
	char response[1024];
	if ((hs = gethostbyname(argv[1])) == NULL) {
		perror("Serveur inconnu");
		exit(2);
	}

	/*Envoi du message*/
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	memcpy(&servaddr.sin_addr.s_addr, hs->h_addr, hs->h_length);

	sendto(fd, argv[3], strlen(argv[3]), 0, (struct sockaddr*)&servaddr, ls);

	/*Réception (attente) de la réponse*/
	memset(response, 0, 1024);
	recvfrom(fd, response, 1024, 0, (struct sockaddr*)&servaddr, &ls);

	/*Sortie*/
	write(STDOUT_FILENO, response, 1024);

	printf("Réponse du serveur : %s\n", response);

}