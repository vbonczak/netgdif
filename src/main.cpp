#include "main.h"
#include "tuto.h"

string nickname;

int main(int argc, char* argv[])
{
	/*Ctrl+C catch*/
	if (sigaction_wrapper(SIGINT, sigint_handler) == -1)
		return EXIT_FAILURE;

	cout << "Entrez votre surnom : " << endl;
	getline(cin, nickname);

	thread back(background);

	string line = "";

	while (!quit)
	{
		getline(cin, line);
		if (quit)
			break;
		if (!line.empty())
		{
			parseLine(line);
		}

		if (!cin)
		{
			quit = true;
		}
	}

	return EXIT_SUCCESS;
}

void parseLine(string line)
{
	if (line[0] == '/')
	{
		string cmd;
		int i = line.find_first_of(' ');
		if (i == line.npos)
			cmd = line;
		else
			cmd = line.substr(0, i);
		i = line.find_first_not_of(' ', i);
		line = line.substr(i);
		if (cmd == "/nick")
		{
			nickname = line;
			cout << "Le surnom a été changé en '" << nickname << "'." << endl;
		}
	}
	else
	{
		sendMessage(nickname + " : " + line);
	}
}

void sendMessage(string msg)
{
	pushTLVToSend(tlvData(myId, msg.c_str(), msg.size(), GetNonce(msg)));
}

void background()
{
	struct sockaddr_in6 servaddr;
	int fd = setup((struct sockaddr_in*)&servaddr);

	copyUUID(RandomBytes(8), myId);

	/*Boucle de service*/
	char message[1024];
	struct sockaddr_in6 client;
	int time = 0;
	int lastNeighbourSentTime = 0;


	while (!quit)
	{
		time = GetTime();

		memset(message, 0, 1024);
		socklen_t len = sizeof(client);
		recvfrom(fd, message, 1024, 0, (struct sockaddr*)&client, &len);

		MIRC_DGRAM dgram;
		parseDatagram(message, 1024, dgram);

		manageDatagram(dgram, mapIPv4((struct sockaddr_in*)&client));

		if (time - lastNeighbourSentTime > NEIGHBOUR_FLOODING_DELAY)
		{
			pushTLVToSend(tlvNeighbour((char*)servaddr.sin6_addr.s6_addr, servaddr.sin6_port));
			lastNeighbourSentTime = time;
		}
	}
}

ADDRESS mapIPv4(struct sockaddr_in* addr)
{
	ADDRESS ret;

	if (addr->sin_family == AF_INET)
	{
		/*Conversion en IPv4 Mapped*/
		char map[16] = { 0,0,0,0,0,0,0,0,0,0,(char)0xff,(char)0xff };
		memcpy(map + 12, (char*)(&(addr->sin_addr.s_addr)), 4);

		memcpy(ret.addrIP, map, 16);
	}
	else
	{
		memcpy(ret.addrIP, (char*)(((struct sockaddr_in6*)addr)->sin6_addr.s6_addr), 16);
	}
	ret.port = addr->sin_port;
	return ret;
}

void manageDatagram(MIRC_DGRAM& dgram, ADDRESS from)
{
	for (auto& entry : dgram)
	{
		switch (entry.first)
		{
		case TLV_HELLO:
			manageHellos(entry.second, from);
			break;
		case TLV_NEIGHBOUR:
			manageNeighbours(entry.second);
			break;
		case TLV_DATA:
			manageDatas(entry.second);
			break;
		case TLV_ACK:
			manageAcks(entry.second);
			break;
		case TLV_GOAWAY:
			manageGoAways(entry.second);
			break;
		case TLV_WARNING:
			manageWarnings(entry.second);
			break;
		default:
			//en principe ils sont ignorés.
			DEBUG("[LOGIC ERROR] Type de TLV ignoré dans la table.");
			break;
		}
	}
}

void manageHellos(list<TLV>& tlvs, ADDRESS from)
{
	for (TLV tlv : tlvs)
		Table_HelloFrom(from, &tlv);
}

void manageDatas(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		tlv.content.data.data[tlv.content.data.dataLen - 1] = '\0';
		cout << "reçu DATA : " << tlv.content.data.data << endl;
	}
}

void manageAcks(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		tlv.content.data.data[tlv.content.data.dataLen - 1] = '\0';
		cout << "reçu DATA : " << tlv.content.data.data << endl;
	}
}

void manageNeighbours(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		cout << "reçu NEIGHBOUR" << endl;
	}
}

void manageGoAways(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		cout << "reçu GOAWAY : " << tlv.content.goAway.code << endl;
	}
}

void manageWarnings(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		tlv.content.data.data[tlv.content.warning.length - 1] = '\0';
		cout << "Warning : " << tlv.content.warning.message << endl;
	}
}

int setup(struct sockaddr_in* addr)
{
	struct sockaddr_in servaddr;
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	/*Connexion à la socket*/
	int fd = NewSocket(AF_INET6, SOCK_DGRAM, 0, (struct sockaddr*)&servaddr);
	if (fd < 0)
	{
		perror("Problème de création de connexion.");
		exit(2);
	}

	/*Coordonnées du serveur*/
	memset(host, 0, NI_MAXHOST);
	memset(service, 0, NI_NAMEREQD);
	if (getnameinfo((struct sockaddr*)&servaddr, sizeof(servaddr), host, NI_MAXHOST, service, NI_MAXSERV, NI_NAMEREQD))
		printf("Serveur lancé à l'adresse %s:%d\n", host, ntohs(servaddr.sin_port));
	else
		printf("Serveur local sur le port %d\n", ntohs(servaddr.sin_port));
	*addr = servaddr;
	return fd;
}

/*
 * wrapper for the sigaction function
 */
int sigaction_wrapper(int signum, handler_t* handler) {
	struct sigaction act;
	sigset_t set;

	act.sa_handler = handler;
	sigemptyset(&set);
	act.sa_mask = set;
	act.sa_flags = SA_RESTART;

	return sigaction(signum, &act, NULL);
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.
 */
void sigint_handler(int sig)
{
	cout << endl << "Exiting." << endl;
	 
	quit = true;

	return;
}