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
#include <signal.h>

/// <summary>
/// Retourne le descripteur de fichier socket correspondant aux informations de connexion.
/// </summary>
/// <param name="domain">Une des constantes de la forme AF_*</param>
/// <param name="type">SOCK_STREAM pour TCP, SOCK_DGRAM pour UDP</param>
/// <param name="port">Le port</param>
/// <param name="p_addr">Champ rempli par cette fonction contenant l'adresse obtenue.</param>
/// <returns></returns>
int NewSocket(int domain, int type, int port, sockaddr* p_addr);

#endif