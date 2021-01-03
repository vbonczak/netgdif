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

	signal(SIGPIPE, SIG_IGN); /*MacOS : Ignorer le fait qu'on écrive dans le vide*/
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

unsigned int IntFromNetwork(unsigned int netlong)
{
	return ntohl(netlong);
}

unsigned int IntToNetwork(unsigned int locallong)
{
	return htonl(locallong);
}

unsigned short ShortFromNetwork(unsigned short netshort)
{
	return ntohs(netshort);
}

unsigned short ShortToNetwork(unsigned short localshort)
{
	return htons(localshort);
}

char* RandomBytes(int size)
{
	char* ret = new char(size);
	for (int i = 0; i < size; i++)
	{
		ret[i] = char(rand() % 256);
	}
	return ret;
}

int RandomInt(int min, int max)
{
	return (rand() % (max - min)) + min;
}

unsigned int GetNonce(string message)
{
	return int(hash<string>()(message + to_string(GetTime())) & 0xffffffff);
}

int GetTime()
{
	using namespace std::chrono;
	static high_resolution_clock::time_point referenceTime = high_resolution_clock::now();
	high_resolution_clock::time_point n = high_resolution_clock::now();
	duration<double, milli> time_span = n - referenceTime;
	return (int)time_span.count();
}


#ifdef VERBOSE
void InitUtils()
{
	srand(static_cast<unsigned int>(time(NULL)));
}
void verbose(char* msg) { printf("DEBUG : %s", msg); }



#endif
