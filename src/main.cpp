#include "main.h"

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
	InitUtils();
	InitMain();

	thread back(background);

	string line;

	while (!quit)
	{
		line = readLine();

		if (quit)
			break;
		if (!line.empty())
		{
			parseLine(line);
		}
	}

	back.join();

	QuitUtils();

	return EXIT_SUCCESS;
}


void InitMain()
{
	//Ctrl+C pour arrêter
	if (sigaction_wrapper(SIGINT, sigint_handler) == -1)
		exit(EXIT_FAILURE);

	copyUUID(RandomBytes(8), myId);

	nickname = readLine("Entrez votre pseudo : ");
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
	pushTLVDATAToFlood(tlvData(myId, msg.c_str(), msg.size() + 1, GetNonce(msg)));
}

void background()
{
	if (quit)
		return;
	struct sockaddr_in6 servaddr;//notre adresse (locale)
	int fd;	//le socket de notre connexion


	int time = 0;
	int lastNeighbourSentTime = 0;
	int lastCleanedTime = 0;
	//Instant de dernier envoi (éviter de surcharger le destinataire)
	int lastSendTime;
	thread* receiver;
	struct sockaddr_in6 client;
	struct sockaddr_in6 myaddr; //notre adresse physique liée à l'interface de communication);
	writeLine("je suis " + UUIDtoString(myId));

	if (setup(&servaddr, fd, multifd, &myaddr) < 0)
	{
		writeErr("Erreur fatale d'initialisation.");
		quit = true;
		return;
	}
	//char str_buffer[150] = { 0 };
	//inet_ntop(AF_INET6, &(myaddr.sin6_addr), str_buffer, 150);
	//DEBUG(str_buffer);
	while (!quit)
	{
		time = GetTime();

		if (!rawUDP_read)
		{
			//le receveur a rempli le paquet
			receiver->join();
			delete receiver;
			/*char res[60];
			inet_ntop(AF_INET6, (void*)(&client.sin6_addr), res, 60);
			writeLine("Reçu de la part de " + string(res) + " le message suivant :");
			writeLine(string(rawUDP));
			*/
			MIRC_DGRAM dgram;
			parseDatagram(rawUDP, rawUDP_len, dgram);
			ADDRESS ad = mapIP((struct sockaddr_in*)&client);

			//inet_ntop(AF_INET6, (void*)(&ad.nativeAddr.sin6_addr), str_buffer, 150);

			manageDatagram(dgram, ad);

			rawUDP_read = true; //nous l'avons lu
		}

		if (!receiving && rawUDP_read)
		{
			//Lance un receveur
			receiver = new thread(receive, fd, &client);
			receiving = true;
		}

		if (time - lastNeighbourSentTime > NEIGHBOUR_FLOODING_DELAY)
		{
			ADDRESS nei = mapIP((struct sockaddr_in*)&myaddr);
			pushTLVToSend(tlvNeighbour(nei.addrIP, nei.port));
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

		if (time - lastSendTime > SEND_PENDING_DELAY)
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

	pushTLVToSend(tlvGoAway(TLV_GOAWAY_QUIT, 22, "Bye bye, see you soon"));
	for (auto& entry : TVA)
	{
		sendPendingTLVs(fd, entry.first);
	}

	shutdown(fd, SHUT_RDWR); //pour que recv retourne immédiatement

	close(multifd);
	close(fd);

	receiver->join();

	mircQuit();

	freeAllTables();
}



void receive(int fd, struct sockaddr_in6* client)
{
	socklen_t len = sizeof(*client);
	rawUDP_len = recvfrom(fd, rawUDP, 1024, 0, (struct sockaddr*)client, &len);
	rawUDP_read = false;
	char* dst = new char[100];

	inet_ntop(AF_INET6, &client->sin6_addr, dst, len);

	DEBUG("Reçu paquet (" + to_string(rawUDP_len) + ") de " + string(dst));
	DEBUGHEX(rawUDP, rawUDP_len);
	delete[] dst;
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

int setup(struct sockaddr_in6* addr, int& fd, int& fd_multicast, struct sockaddr_in6* physaddr)
{
	struct sockaddr_in6 servaddr;
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	/*Connexion à la socket*/
		//La variable globale multifd sert à envoyer des paquets en multicast ipv6
	fd = NewSocket(SOCK_DGRAM, MULTICAST_PORT, (struct sockaddr*)&servaddr, multifd, physaddr);
	if (fd < 0)
	{
		writeErr("Problème de création de connexion.");
		return -1;
	}

	/*Coordonnées du serveur*/
	memset(host, 0, NI_MAXHOST);
	memset(service, 0, NI_NAMEREQD);
	if (getnameinfo((struct sockaddr*)&servaddr, sizeof(servaddr), host, NI_MAXHOST, service, NI_MAXSERV, NI_NAMEREQD))
	{
		char* dst = new char[100];
		socklen_t len = sizeof(servaddr);
		if (inet_ntop(AF_INET6, &servaddr.sin6_addr, dst, len) == NULL)
			writeErr("Erreur");
	} 
	*addr = servaddr;

	return 0;
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
