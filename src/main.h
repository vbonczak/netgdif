#include <iostream>
#include "tables.h"
#include "utils.h"
#include <signal.h>
#include "unistd.h"
#include "mirc.h"
#include <thread>

bool quit = false;
typedef void handler_t(int);
using namespace std;

void parseLine(string line);

void sendMessage(string msg);

void background();

void receive(int fd);

/// <summary>
/// Encapsule l'adresse physique (IP4 ou 6) dans notre structure ADDRESS.
/// </summary>
/// <param name="addr"></param>
/// <returns></returns>
ADDRESS mapIP(sockaddr_in* addr);

/// <summary>
/// Mappe au bon endroit l'adresse et le port dans la structure native sockaddr_in6
/// </summary>
/// <param name="ipv6"></param>
/// <param name="port"></param>
struct sockaddr_in6 address2IP(char ipv6[16], unsigned short port);

void manageDatagram(MIRC_DGRAM& dgram, ADDRESS& from);

void manageNeighbours(list<TLV>& tlvs);
void manageHellos(list<TLV>& tlvs, ADDRESS& from);
void manageDatas(list<TLV>& tlvs, ADDRESS& from);
void manageAcks(list<TLV>& tlvs, ADDRESS& from);
void manageGoAways(list<TLV>& tlvs, ADDRESS& from);
void manageWarnings(list<TLV>& tlvs);
int setup(struct sockaddr_in6*);

int sigaction_wrapper(int signum, handler_t* handler);

void sigint_handler(int sig);
