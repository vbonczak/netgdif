#include "utils.h"

int multifd = -1;
string multiIP = "ff02::4242:4242";
struct sockaddr_in6 multiaddr;
char* textBuffer;

//Affichage défilant
WINDOW* winput, * woutput;
int row = 0, col = 0, oldrow = 0;

int NewSocket(int type, int port, struct sockaddr* p_addr, int& fd_multicast, struct sockaddr_in6* physicaladdr)
{
	/*// OPEN
	int fd = socket(AF_INET6, SOCK_DGRAM, 0);

	// BIND
	struct sockaddr_in6 address = { AF_INET6, htons(MULTICAST_PORT) };
	bind(fd, (struct sockaddr*)&address, sizeof address);

	// JOIN MEMBERSHIP
	struct ipv6_mreq group;
	group.ipv6mr_interface = 0;
	inet_pton(AF_INET6, multiIP.c_str(), &group.ipv6mr_multiaddr);
	setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &group, sizeof group);

	return fd;*/

	int fd = socket(AF_INET6, type, 0);

	if (fd < 0)
	{
		writeErr("Creation de socket impossible");
		return -1;
	}

	/*Réutilisation d'une adresse existante*/
	int optval = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	signal(SIGPIPE, SIG_IGN); /*MacOS : Ignorer le fait qu'on écrive dans le vide*/
	socklen_t len_p_addr = 0;

	struct sockaddr_in6 addr;
	len_p_addr = sizeof(struct sockaddr_in6);
	memset(&addr, 0, len_p_addr); //INADDR_ANY
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port); /*Conversion de boutisme*/


	if (setupMulticast(fd, fd_multicast, physicaladdr) != 0)
	{
		//Multicast non disponible
		fd_multicast = -1;
	}

	//Multicast
	if (bind(fd, (struct sockaddr*)&addr, len_p_addr) == -1) {
		writeErr("Attachement socket IPv6 impossible");
		return -1;
	}

	if (p_addr != NULL)
		getsockname(fd, (struct sockaddr*)p_addr, &len_p_addr);

	return fd;
}


int setupMulticast(int fd, int& fd_multicast, struct sockaddr_in6* myInterfaceAddr)
{
	/*Connexion à la socket*/
	fd_multicast = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	if (fd_multicast < 0)
	{
		writeErr("Creation de socket impossible");
		return -1;
	}

	/*Configuration du socket multicast*/
	int optval = 1;
	setsockopt(fd_multicast, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(fd_multicast, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval));
	setsockopt(fd_multicast, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &optval, sizeof(optval));
	setsockopt(fd_multicast, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &optval, sizeof(optval));
	optval = 0;
	setsockopt(fd_multicast, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optval, sizeof(optval));


	signal(SIGPIPE, SIG_IGN); /*MacOS : Ignorer le fait qu'on écrive dans le vide*/
	socklen_t len_p_addr = 0;


	len_p_addr = sizeof(struct sockaddr_in6);
	inet_pton(AF_INET6, multiIP.c_str(), &(multiaddr.sin6_addr));
	multiaddr.sin6_family = AF_INET6;
	multiaddr.sin6_port = htons(MULTICAST_PORT); /*Conversion de boutisme*/
	multiaddr.sin6_scope_id = findMulticastInterface(myInterfaceAddr);

	if (bind(fd_multicast, (struct sockaddr*)&multiaddr, len_p_addr) == -1) {
		writeErr("Attachement socket multicast IPv6 impossible");
		return -1;
	}

	/*Joindre le groupe multicast*/
	struct ipv6_mreq multicastRequest;

	memcpy(&multicastRequest.ipv6mr_multiaddr, &multiaddr.sin6_addr, sizeof(multicastRequest.ipv6mr_multiaddr));

	//Même interface que le socket fd_multicast
	multicastRequest.ipv6mr_interface = multiaddr.sin6_scope_id;
	/* Join the multicast address */
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) != 0)
	{
		writeErr("Problème d'inscription au groupe multicast.");
	}

	return 0;
}

int findMulticastInterface(struct sockaddr_in6* interfaceAddr)
{
	struct ifaddrs* ifaddr = NULL;

	if (getifaddrs(&ifaddr) || ifaddr == NULL)
	{
		writeErr("Erreur lors de la recherche d'interface pour le Multicast");
		return 0;
	}
	struct ifaddrs* cur = ifaddr;

	char* name = NULL;
	while (cur != NULL)
	{

		if ((cur->ifa_flags & IFF_MULTICAST) != 0 && (cur->ifa_flags & IFF_UP) != 0 && (cur->ifa_addr->sa_family == AF_INET6))
		{
			//OK pour multicast
			name = cur->ifa_name;
			*interfaceAddr = *(struct sockaddr_in6*)cur->ifa_addr;
			break;
		}
		cur = cur->ifa_next;
	}
	int ret = 0;
	if (name != NULL)
		ret = if_nametoindex(name);

	DEBUG("Interface sélectionnée : " + string(name));

	freeifaddrs(ifaddr);

	return ret;
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
	char* ret = new char[size];
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
	textBuffer = new char[256];

	//Affichage avec curses
	initscr();
	woutput = newwin(LINES - 4, COLS, 0, 0);
	winput = newwin(3, COLS, LINES - 4, 0);
	scrollok(woutput, true);
}

void QuitUtils()
{
	delwin(winput);
	delwin(woutput);
	endwin();
	delete[] textBuffer;
}

struct sockaddr_in6 address2IP(char ipv6[16], unsigned short port)
{
	struct sockaddr_in6 ret;
	bool zeros = true;
	for (int i = 0; i < 10; i++)
	{
		zeros = zeros && (ipv6[i] == 0);
	}
	if (zeros && ipv6[10] == 0xff && ipv6[11] == 0xff)
	{
		//C'est une ipv4 Mapped
		struct sockaddr_in* ret4 = (struct sockaddr_in*)(&ret);
		ret4->sin_family = AF_INET;
		ret4->sin_port = ShortToNetwork(port);//Boutisme réseau pour le port dans la structure native.
		string ip = to_string(ipv6[12]) + "." + to_string(ipv6[13]) + "." + to_string(ipv6[14]) + "." + to_string(ipv6[15]);
		inet_aton(ip.c_str(), &(ret4->sin_addr));
	}
	else
	{
		//ret.sin6_len = sizeof(ret); //According to other standards than POSIX
		ret.sin6_family = AF_INET6;
		memcpy(ret.sin6_addr.s6_addr, ipv6, 16);
		ret.sin6_port = ShortToNetwork(port);//Boutisme réseau pour le port dans la structure native.
	}
	return ret;
}



void writeLine(string line)
{
	waddstr(woutput, (line + "\n").c_str());
	wrefresh(woutput);
}

void writeErr(string line)
{
	writeLine(line + " : " + string(strerror(errno)));
}

string readLine()
{
	wgetstr(winput, textBuffer);
	wclear(winput);
	wrefresh(winput);

	return string(textBuffer);
}

string readLine(string inputText)
{
	waddstr(winput, inputText.c_str());
	wrefresh(winput);
	wgetstr(winput, textBuffer);
	wclear(winput);
	wrefresh(winput);
	return string(textBuffer);
}

void refreshWin()
{
	getmaxyx(stdscr, row, col);
	if (row != oldrow)
	{
		wresize(woutput, row - 4, col);
		mvwin(winput, row - 4, 0);
		wmove(winput, 0, 0);
		oldrow = row;
	}
}

#ifdef VERBOSE

void verbose(string msg) {
	writeLine("[Debug t = " + to_string(
		GetTime()) + "]" + msg + "\n");
}

void verbosehex(char* data, int l)
{
	string r = "";
	for (int i = 0; i < l; i++)
	{
		r += to_string((unsigned char)data[i]) + " ";
	}
	writeLine(r);
}

#endif
