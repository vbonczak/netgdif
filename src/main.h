#include <iostream>
#include "tables.h"
#include "utils.h"
#include <signal.h>
#include "unistd.h"
#include "mirc.h"
#include <thread>

using namespace std;



/// <summary>
/// Variable accédée par les deux threads, mais par un seul en écriture.
/// </summary>
bool quit = false;

/// <summary>
/// Gestion de l'interruption au clavier.
/// </summary>
typedef void handler_t(int);

/// <summary>
/// Effectue les initialisations au début du programme, comme l'ID unique du pair, et son pseudo.
/// </summary>
void InitMain();

/// <summary>
/// Permet de gérer l'entrée du pair dans la console.
/// </summary>
/// <param name="line"></param>
void parseLine(string line);

/// <summary>
/// Envoyer un message dans le groupe.
/// </summary>
/// <param name="msg"></param>
void sendMessage(string msg);

/// <summary>
/// Le fil d'exécution d'arrière plan.
/// </summary>
void background();

/// <summary>
/// 
/// </summary>
/// <param name="recvThread"></param>
/// <param name="socket"></param>
/// <param name="readFlag"></param>
/// <param name="receivingFlag"></param>
/// <param name="recvLen"></param>
/// <param name="rawUDP"></param>
/// <param name="sender"></param>
void doReceive(thread** recvThread, int socket, bool* readFlag, bool* receivingFlag, int* recvLen, char* rawUDP, sockaddr_in6* sender);

/// <summary>
/// Fil d'exécution chargé d'attendre un paquet
/// </summary>
/// <param name="fd"></param>
/// <param name="client"></param>
/// <param name="readFlag"></param>
/// <param name="receivingFlag"></param>
/// <param name="recvLen"></param>
/// <param name="rawUDP"></param>
void receive(int fd, sockaddr_in6* client, bool* readFlag, bool* receivingFlag, int* recvLen, char* rawUDP);
 

/// <summary>
/// Encapsule l'adresse physique (IP4 ou 6) dans notre structure ADDRESS.
/// </summary>
/// <param name="addr"></param>
/// <returns></returns>
ADDRESS mapIP(sockaddr_in* addr);

/// <summary>
/// Gestion des paquets UDP entrants
/// </summary>
/// <param name="dgram"></param>
/// <param name="from"></param>
void manageDatagram(MIRC_DGRAM& dgram, ADDRESS& from);

/*
Les fonctions suivantes sont chacune dédiées à un certain type de TLV, traitées ensemble par type.
*/
void manageNeighbours(list<TLV>& tlvs);
void manageHellos(list<TLV>& tlvs, ADDRESS& from);
void manageDatas(list<TLV>& tlvs, ADDRESS& from);
void manageAcks(list<TLV>& tlvs, ADDRESS& from);
void manageGoAways(list<TLV>& tlvs, ADDRESS& from);
void manageWarnings(list<TLV>& tlvs);

/// <summary>
/// Établissement des connexion, gestion multicast.
/// </summary>
/// <param name="serv"></param>
/// <param name="fd"></param>
/// <param name="mfd"></param>
/// <param name="physaddr"></param>
/// <returns></returns>
int setup(struct sockaddr_in6* serv, int& fd, int& mfd, struct sockaddr_in6* physaddr);

/// <summary>
/// Le CTRL+C
/// </summary>
/// <param name="signum"></param>
/// <param name="handler"></param>
/// <returns></returns>
int sigaction_wrapper(int signum, handler_t* handler);

/// <summary>
/// Le CTRL+C
/// </summary>
/// <param name="sig"></param>
void sigint_handler(int sig);
