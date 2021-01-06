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

void receive(int fd, sockaddr_in6* client);

/// <summary>
/// Encapsule l'adresse physique (IP4 ou 6) dans notre structure ADDRESS.
/// </summary>
/// <param name="addr"></param>
/// <returns></returns>
ADDRESS mapIP(sockaddr_in* addr);



void manageDatagram(MIRC_DGRAM& dgram, ADDRESS& from);

void manageNeighbours(list<TLV>& tlvs);
void manageHellos(list<TLV>& tlvs, ADDRESS& from);
void manageDatas(list<TLV>& tlvs, ADDRESS& from);
void manageAcks(list<TLV>& tlvs, ADDRESS& from);
void manageGoAways(list<TLV>& tlvs, ADDRESS& from);
void manageWarnings(list<TLV>& tlvs);


int setup(struct sockaddr_in6*, int& fd, int& mfd);

int sigaction_wrapper(int signum, handler_t* handler);

void sigint_handler(int sig);
