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

ADDRESS mapIPv4(sockaddr_in* addr);

void manageDatagram(MIRC_DGRAM& dgram, ADDRESS from);

void manageNeighbours(list<TLV>& tlvs);
void manageHellos(list<TLV>& tlvs, ADDRESS from);
void manageDatas(list<TLV>& tlvs);
void manageAcks(list<TLV>& tlvs);
void manageGoAways(list<TLV>& tlvs);
void manageWarnings(list<TLV>& tlvs);
int setup(struct sockaddr_in6*);

int sigaction_wrapper(int signum, handler_t* handler);

void sigint_handler(int sig);
