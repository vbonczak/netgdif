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

int RandomInt(int min, int max);

/// <summary>
/// 
/// </summary>
/// <param name="message"></param>
/// <returns></returns>
unsigned int GetNonce(string message);

#endif