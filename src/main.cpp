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
	writeLine("Entrez votre pseudo : ");
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
			writeLine("Le surnom a été changé en '" + nickname + "'.");
		}
	}
	else
	{
		sendMessage(nickname + " : " + line);
	}
}

void sendMessage(string msg)
{
	//pushTLVDATAToFlood(tlvData(myId, msg.c_str(), msg.size(), GetNonce(msg)));
	socklen_t l = sizeof(multiaddr);
	sendto(multifd, msg.c_str(), msg.size() + 1, 0, (struct sockaddr*)(&multiaddr), l);
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
	struct sockaddr_in6 client;
	while (!quit)
	{
		writeLine("tour de boucle");

		time = GetTime();

		if (!rawUDP_read)
		{
			//le receveur a rempli le paquet
			receiver->join();
			delete receiver;
			char res[60];
			inet_ntop(AF_INET6, (void*)(&client.sin6_addr), res, 60);
			writeLine("Reçu de la part de " + string(res) + " le message suivant :");
			writeLine(string(rawUDP));
			/*
			MIRC_DGRAM dgram;
			parseDatagram(rawUDP, rawUDP_len, dgram);
			ADDRESS ad = mapIP((struct sockaddr_in*)&client);
			manageDatagram(dgram, ad);
			*/
			rawUDP_read = true; //nous l'avons lu
		}

		if (!receiving && rawUDP_read)
		{
			//Lance un receveur
			receiver = new thread(receive, fd, &client);
			receiving = true;
		}
		/*
		if (time - lastNeighbourSentTime > NEIGHBOUR_FLOODING_DELAY)
		{
			pushTLVToSend(tlvNeighbour((char*)servaddr.sin6_addr.s6_addr, servaddr.sin6_port));
			//Voisins symétriques
			for (auto& entry : TVA)
			{
				if (entry.second.symmetrical)
					pushTLVToSend(tlvNeighbour(entry.first.addrIP, entry.first.port));
			}
			lastNeighbourSentTime = time;
		}

		//Toutes les 2mn, on nettoie les données reçues
		if (time - lastCleanedTime > 2 * MINUTE)
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
			writeLine("Envoi des TLVs en attente");
			lastSendTime = time;
			//Envoi effectif des paquets UDP vers leurs destinataires respectifs.
			for (auto& entry : TVA)
			{
				sendPendingTLVs(fd, entry.first);
			}
		}
		*/
		sleep(1);
	}

	shutdown(fd, SHUT_RDWR);

	receiver->join();

	close(fd);

	mircQuit();

	freeAllTables();
}



void receive(int fd, struct sockaddr_in6* client)
{
	socklen_t len = sizeof(*client);
	rawUDP_len = recvfrom(fd, rawUDP, 1024, 0, (struct sockaddr*)client, &len);
	rawUDP_read = false;

	//sendto(fd, "Recu", 5, 0, (struct sockaddr*)client, sizeof(*client));
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
	ret.port = ntohs(addr->sin_port);//Boutisme hôte pour les champs managés
	//On conserve l'adresse native pour aller plus vite
	ret.nativeAddr = *((struct sockaddr_in6*)addr);
	return ret;
}

struct sockaddr_in6 address2IP(char ipv6[16], unsigned short port)
{
	struct sockaddr_in6 ret;
	bool zeros = true;
	for (int i = 0; i < 10; i++)
	{
		zeros = zeros && (ipv6[i] == 0);
	}
	if (zeros && ipv6[10] == 0xff && ipv6[10] == 0xff)
	{
		//C'est une ipv4 Mapped
		struct sockaddr_in* ret4 = (struct sockaddr_in*)(&ret);
		ret4->sin_family = AF_INET;
		ret4->sin_port = htons(port);//Boutisme réseau pour le port dans la structure native.
		string ip = to_string(ipv6[12]) + "." + to_string(ipv6[13]) + "." + to_string(ipv6[14]) + "." + to_string(ipv6[15]);
		inet_aton(ip.c_str(), &(ret4->sin_addr));
	}
	else
	{
		//ret.sin6_len = sizeof(ret); //According to other standards than POSIX
		ret.sin6_family = AF_INET6;
		memcpy(ret.sin6_addr.s6_addr, ipv6, 16);
		ret.sin6_port = htons(port);//Boutisme réseau pour le port dans la structure native.
	}
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
		Table_HelloFrom(from, tlv);
}

void manageDatas(list<TLV>& tlvs, ADDRESS& from)
{
	for (TLV tlv : tlvs)
	{
		//tlv.content.data.data[tlv.content.data.dataLen - 1] = '\0';
		//cout << "reçu DATA : " << tlv.content.data.data << endl;
		Table_DataFrom(tlv, from);
	}
}

void manageAcks(list<TLV>& tlvs, ADDRESS& from)
{
	for (TLV tlv : tlvs)
	{
		Table_ACKFrom(tlv, from);
	}
}

void manageNeighbours(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		ADDRESS toAdd;
		memcpy(toAdd.addrIP, tlv.content.neighbour.addrIP, 16);
		toAdd.port = tlv.content.neighbour.port;
		if (TVP.find(toAdd) == TVP.end()) {
			address2IP(tlv.content.neighbour.addrIP, tlv.content.neighbour.port);
			//Ajout à TVP d'un ID inconnu, qui sera identifié lors d'un Hello éventuel venant de cette adresse IP.
			TVP[toAdd];
		}
	}
}

void manageGoAways(list<TLV>& tlvs, ADDRESS& from)
{
	for (TLV tlv : tlvs)
	{
		eraseFromTVA(from);

		freeTLV(tlv);
	}
}

void manageWarnings(list<TLV>& tlvs)
{
	for (TLV tlv : tlvs)
	{
		tlv.content.data.data[tlv.content.warning.length - 1] = '\0';
		writeLine("Warning : " + string(tlv.content.warning.message));
		freeTLV(tlv);
	}
}

int setup(struct sockaddr_in6* addr)
{
	struct sockaddr_in6 servaddr;
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	/*Connexion à la socket*/
	int fd = NewSocket(SOCK_DGRAM, MULTICAST_PORT, (struct sockaddr*)&servaddr);
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
			writeErr(strerror(errno));

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
	writeLine("\nArrêt. Appuyez sur Entrée pour quitter.");

	quit = true;

	return;
}
