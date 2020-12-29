#include <iostream>
#include "tables.h"
#include "utils.h"
#include <signal.h>
#include "unistd.h"
#include "mirc.h"

bool quit = false;
typedef void handler_t(int);
using namespace std;

int setup(struct sockaddr_in*);

int sigaction_wrapper(int signum, handler_t* handler);

void sigint_handler(int sig);
