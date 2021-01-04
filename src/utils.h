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

using namespace std;

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

void InitUtils();

#define VERBOSE

#ifdef VERBOSE
#define DEBUG(x)	verbose(x)
void verbose(string msg);
#else
#define DEBUG(x)	 
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
/// <param name="type">SOCK_STREAM pour TCP, SOCK_DGRAM pour UDP</param>
/// <param name="port">Le port</param>
/// <param name="p_addr">Champ rempli par cette fonction contenant l'adresse obtenue.</param>
/// <returns></returns>
int NewSocket(int domain, int type, int port, sockaddr* p_addr);

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

#endif