#include "utils.h"

int multifd = -1;
string multiIP = "ff02::4242:4242";
struct sockaddr_in6 multiaddr;

int NewSocket(int domain, int type, int port, struct sockaddr* p_addr)
{
	int fd = socket(domain, type, IPPROTO_UDP);

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
		memset(&addr, 0, len_p_addr); //INADDR_ANY
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


int setupMulticast()
{
	/*Connexion à la socket*/
	int fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	if (fd < 0)
	{
		perror("Creation de socket impossible");
		return -1;
	}

	/*Configuration du socket multicast*/
	int optval = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(fd, SOL_SOCKET, IPV6_V6ONLY, &optval, sizeof(optval));
	setsockopt(fd, SOL_SOCKET, IPV6_MULTICAST_HOPS, &optval, sizeof(optval));
	setsockopt(fd, SOL_SOCKET, IPV6_UNICAST_HOPS, &optval, sizeof(optval));
	optval = 0;
	setsockopt(fd, SOL_SOCKET, IPV6_MULTICAST_LOOP, &optval, sizeof(optval));

	signal(SIGPIPE, SIG_IGN); /*MacOS : Ignorer le fait qu'on écrive dans le vide*/
	socklen_t len_p_addr = 0;

	 
	len_p_addr = sizeof(struct sockaddr_in6);
	inet_pton(AF_INET6, multiIP.c_str(), &(multiaddr.sin6_addr));
	multiaddr.sin6_family = AF_INET6;
	multiaddr.sin6_port = htons(MULTICAST_PORT); /*Conversion de boutisme*/

	if (bind(fd, (struct sockaddr*)&multiaddr, len_p_addr) == -1) {
		perror("Attachement socket IPv6 impossible");
		return -1;
	}
	multifd = fd;
	return 0;
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

bool tabEq(const char* a, const char* b, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (a[i] != b[i])
			return false;
	}
	return true;
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

void InitUtils()
{
	srand(static_cast<unsigned int>(time(NULL)));
}


void writeLine(string line)
{
	cout << line << endl;
}

void writeErr(string line)
{
	cout << line << endl;
}

#ifdef VERBOSE

void verbose(string msg) { cout << "DEBUG : " << msg << endl; }



#endif
