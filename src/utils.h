#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <string>
#include <signal.h>
#include <arpa/inet.h>
#include <chrono>
#include <net/if.h>
#include <ifaddrs.h>

#include <ncurses.h>

using namespace std;

#define MULTICAST_PORT	1212
extern int multifd;
extern int fd;
extern string multiIP;
extern struct sockaddr_in6 multiaddr;

bool tabEq(const char* a, const char* b, int size = 16);

typedef struct ADDRESS_s
{
	char addrIP[16];
	unsigned short port;

	struct sockaddr_in6 nativeAddr;

	bool const operator==(const struct ADDRESS_s& o) const
	{
		return port == o.port && tabEq(addrIP, o.addrIP);
	}

	bool const operator!=(const struct ADDRESS_s& o) const
	{
		return !(o == *this);
	}
} ADDRESS;

struct ADDRESSHash
{
	size_t operator()(const ADDRESS& o) const
	{
		return hash<unsigned short>()(o.port) ^ hash<uint64_t>()(*((uint64_t*)(o.addrIP))) ^ hash<uint64_t>()(*((uint64_t*)(o.addrIP + 8)));
	}
};

/// <summary>
/// Mappe au bon endroit l'adresse et le port dans la structure native sockaddr_in6
/// </summary>
/// <param name="ipv6"></param>
/// <param name="port"></param>
struct sockaddr_in6 address2IP(char ipv6[16], unsigned short port);

void InitUtils();

void QuitUtils();

#ifdef VERBOSE
#define DEBUG(x)	verbose(x)
#define DEBUGHEX(x,y)	verbosehex(x,y)
void verbose(string msg);
void verbosehex(char* data, int l);
#else
#define DEBUG(x)	
#define DEBUGHEX(x,y)
#endif



#define MINUTE	60000



/// <summary>
/// Millisecondes depuis le début de l'exécution.
/// </summary>
/// <returns></returns>
int GetTime();

/// <summary>
/// Retourne le descripteur de fichier socket correspondant aux informations de connexion.
/// </summary>
/// <param name="domain">Une des constantes de la forme AF_*</param>
/// <param name="port">Le port</param>
/// <param name="p_addr">Champ rempli par cette fonction contenant l'adresse obtenue.</param>
/// <param name="mfd">Descripteur de fichier correspondant au socket multicast.</param>
/// <param name="physaddr">L'adresse physique de notre interface.</param>
/// <returns></returns>
int NewSocket(int type, int port, struct sockaddr* p_addr, int& mfd,struct sockaddr_in6* physaddr);



/// <summary>
/// 
/// </summary>
/// <param name="netlong"></param>
/// <returns></returns>
unsigned int IntFromNetwork(unsigned int netlong);

unsigned int IntToNetwork(unsigned int locallong);

unsigned short ShortFromNetwork(unsigned short netshort);

unsigned short ShortToNetwork(unsigned short localshort);

char* RandomBytes(int size);

/// <summary>
/// Créée un socket multicast pour l'envoi des Hellos, et connecte le socket standard
/// pour recevoir des messages du même groupe multicast.
/// </summary>
/// <param name="fd"></param>
/// <returns></returns>
int setupMulticast(int fd, int& fd_multicast, struct sockaddr_in6* myInterfaceAddr);

/// <summary>
/// 
/// </summary>
/// <param name="interfaceAddr"></param>
/// <returns></returns>
int findMulticastInterface(struct sockaddr_in6* interfaceAddr);

/// <summary>
/// Retourne un entier aléatoire entre min et max.
/// </summary>
/// <param name="min"></param>
/// <param name="max"></param>
/// <returns></returns>
int RandomInt(int min, int max);

/// <summary>
/// 
/// </summary>
/// <param name="message"></param>
/// <returns></returns>
unsigned int GetNonce(string message);
void writeLine(string line);

void writeErr(string line);
string readLine();
string readLine(string inputText);
void refreshWin();

string EatToken(string& line, char sep=' ');

extern WINDOW* winput;
extern WINDOW* woutput;
#endif