#include "utils.h"

int NewSocket(int domain, int type, int port, struct sockaddr* p_addr)
{
	int fd = socket(domain, type, 0);

	if (fd < 0)
	{
		perror("Creation de socket impossible");
		return -1;
	}

	/*Réutilisation d'une adresse existante*/
	int optval = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	//signal(SIGPIPE, SIG_IGN); /*Ignorer le fait qu'on écrive dans le vide*/

	socklen_t len_p_addr = 0;

	/*Bind l'adresse selon IPv4 ou IPv6*/
	if (domain == AF_INET)
	{
		struct sockaddr_in addr; 
		len_p_addr = sizeof(struct sockaddr_in);
		memset(&addr, 0, len_p_addr);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);  /*Conversion de boutisme*/
	 
		if (bind(fd, (struct sockaddr*)&addr, len_p_addr) == -1)
		{
			perror("Attachement socket IPv4 impossible");
			return -1;
		}
	}
	else
	{
		struct sockaddr_in6 addr;
		len_p_addr = sizeof(struct sockaddr_in6);
		memset(&addr, 0, len_p_addr);
		addr.sin6_family = AF_INET6;
		addr.sin6_port = htons(port); /*Conversion de boutisme*/
		 
		if (bind(fd, (struct sockaddr*)&addr, len_p_addr) == -1) {
			perror("Attachement socket IPv6 impossible");
			return -1;
		}
	}

	if (p_addr != NULL)
		getsockname(fd, (struct sockaddr*)p_addr, &len_p_addr);

	return fd;
}