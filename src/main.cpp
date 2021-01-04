#include "main.h"
#include "tuto.h"

/*Quelques variables globales*/

//Le surnom
string nickname;

//Gestion de reception des paquets en parallèle, pour éviter un blocage
char rawUDP[1024];
int rawUDP_len = 0;
bool rawUDP_read = true;
bool receiving = false;

int main(int argc, char* argv[])
{
	/*Ctrl+C catch*/
	if (sigaction_wrapper(SIGINT, sigint_handler) == -1)
		return EXIT_FAILURE;

	copyUUID(RandomBytes(8), myId);
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

	back.join();

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
	pushTLVDATAToFlood(tlvData(myId, msg.c_str(), msg.size(), GetNonce(msg)));
}

void background()
{
	struct sockaddr_in6 servaddr;
	int fd = setup(&servaddr);

	int time = 0;
	int lastNeighbourSentTime = 0;
	int lastCleanedTime = 0;
	//Instant de dernier envoi (éviter de surcharger le destinataire)
	int lastSendTime;
	thread* receiver;
	while (!quit)
	{
		cout << "tour de boucle" << endl;
		time = GetTime();

		if (!receiving)
		{
			//Lance un receveur
			receiver = new thread(receive, fd);
			receiving = true;
		}

		if (!rawUDP_read)
		{
			//le receveur a rempli le paquet
			receiver->join();
			delete receiver;

			MIRC_DGRAM dgram;
			parseDatagram(rawUDP, rawUDP_len, dgram);
			ADDRESS ad = mapIP((struct sockaddr_in*)&client);
			manageDatagram(dgram, ad);
			rawUDP_read = true; //nous l'avons lu
		}

		if (time - lastNeighbourSentTime > NEIGHBOUR_FLOODING_DELAY)
		{
			pushTLVToSend(tlvNeighbour((char*)servaddr.sin6_addr.s6_addr, servaddr.sin6_port));
			lastNeighbourSentTime = time;
		}

		//Toutes les 30sec, on nettoie les données reçues
		if (time - lastCleanedTime > 30000)
		{
			Table_CleanRR();
			lastCleanedTime = time;
		}

		//Mise à jour de la liste des voisins actifs
		Table_RefreshTVA();
		//Remplissage des messages de l'utilisateur courant
		pushPendingForFlood();

		if (time - lastSendTime > 5000)
		{
			lastSendTime = time;
			//Envoi effectif des paquets UDP vers leurs destinataires respectifs.
			for (auto& entry : TVA)
			{
				sendPendingTLVs(fd, entry.first);
			}
		}
		sleep(1);
	}

	shutdown(fd, SHUT_RDWR);

	receiver->join();

	close(fd);
}



void receive(int fd)
{
	struct sockaddr_in6 client;
	socklen_t len = sizeof(client);
	rawUDP_len = recvfrom(fd, rawUDP, 1024, 0, (struct sockaddr*)&client, &len);
	if (rawUDP_len > 0)
		rawUDP_read = false;

	receiving = false;
}

ADDRESS mapIP(struct sockaddr_in* addr)
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
	//On conserve l'adresse native pour aller plus vite
	ret.nativeAddr = *((struct sockaddr_in6*)addr);
	return ret;
}

struct sockaddr_in6 address2IP(char ipv6[16], unsigned short port)
{
	struct sockaddr_in6 ret;

	memcpy(ret.sin6_addr.s6_addr, ipv6, 16);
	ret.sin6_port = port;
	return ret;
}

void manageDatagram(MIRC_DGRAM& dgram, ADDRESS& from)
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
			manageDatas(entry.second, from);
			break;
		case TLV_ACK:
			manageAcks(entry.second, from);
			break;
		case TLV_GOAWAY:
			manageGoAways(entry.second, from);
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

void manageHellos(list<TLV>& tlvs, ADDRESS& from)
{
	for (TLV tlv : tlvs)
		Table_HelloFrom(from, &tlv);
}

void manageDatas(list<TLV>& tlvs, ADDRESS& from)
{
	for (TLV tlv : tlvs)
	{
		//tlv.content.data.data[tlv.content.data.dataLen - 1] = '\0';
		//cout << "reçu DATA : " << tlv.content.data.data << endl;
		Table_DataFrom(&tlv, from);
	}
}

void manageAcks(list<TLV>& tlvs, ADDRESS& from)
{
	for (TLV tlv : tlvs)
	{
		Table_ACKFrom(&tlv, from);
	}
}

void manageNeighbours(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		ADDRESS toAdd;
		memcpy(toAdd.addrIP, tlv.content.neighbour.addrIP, 16);
		toAdd.port = tlv.content.neighbour.port;
		address2IP(tlv.content.neighbour.addrIP, tlv.content.neighbour.port);
		//Ajout à TVP d'un ID inconnu, qui sera identifié lors d'un Hello éventuel venant de cette adresse IP.
		TVP[toAdd];
	}
}

void manageGoAways(list<TLV>& tlvs, ADDRESS& from)
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

int setup(struct sockaddr_in6* addr)
{
	struct sockaddr_in6 servaddr;
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
	{
		char* dst = new char[100];
		socklen_t len = sizeof(servaddr);
		if (inet_ntop(AF_INET6, &servaddr.sin6_addr, dst, len) == NULL)
			cout << strerror(errno) << endl;

		printf("Serveur lancé à l'adresse %s:%d\n", dst, ntohs(servaddr.sin6_port));
	}
	else
		printf("Serveur local sur le port %d\n", ntohs(servaddr.sin6_port));
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
	cout << endl << "Exiting. You may have to type enter again." << endl;

	quit = true;

	return;
}