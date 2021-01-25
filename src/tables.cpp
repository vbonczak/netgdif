#include "tables.h"
#include "mirc.h"

unordered_map<ADDRESS, UUID, ADDRESSHash> TVP;

unordered_map<ADDRESS, INFOPAIR, ADDRESSHash> TVA;

unordered_map<DATAID, DATAINFO, DATAIDHash> RR;

int MAX_RR = 1000;
int DATA_LIFETIME = 3 * MINUTE;

void Table_HelloFrom(ADDRESS& addr, TLV helloTLV)
{
	if (equalsUUID(helloTLV.content.hello.sourceID, myId))
	{
		//Reçu de nous même dans le multicast (ne doit pas arriver)
		DEBUG("reçu de nous même");
		return;
	}

	//On identifie l'ID de notre voisin potentiel reçu de NEIGHBOUR précédemment
	copyUUID(helloTLV.content.hello.sourceID, TVP[addr]);

	int time = GetTime();
	if (TVA.find(addr) == TVA.end())
	{
		//Pas encore connu comme voisin actif
		TVA[addr] = INFOPAIR();
		copyUUID(helloTLV.content.hello.sourceID, TVA[addr].id);
		if (helloTLV.content.hello.longFormat)
		{
			if (equalsUUID(myId, helloTLV.content.hello.destID))
			{
				//Symétrique
				TVA[addr].symmetrical = true;
				DEBUG("Le voisin " + UUIDtoString(TVA[addr].id) + " a été marqué comme symétrique.");
			}
		}
		else
		{
			TVA[addr].lastHello16Date = 0;//Au début, jamais reçu de long
			pushTLVToSend(tlvHello(myId, helloTLV.content.hello.sourceID));//envoi d'un long pour faire connaissance
		}
	}
	//Mise à jour des instants de réception des hellos.
	TVA[addr].lastHelloDate = time;
	if (helloTLV.content.hello.longFormat && equalsUUID(myId, helloTLV.content.hello.destID))
	{
		TVA[addr].lastHello16Date = time;
		DEBUG("Le voisin " + UUIDtoString(TVA[addr].id) + " a été marqué comme symétrique.");
		TVA[addr].symmetrical = true;
	}

	freeTLV(helloTLV);
}

void Table_DataFrom(TLV dataTLV, ADDRESS& from)
{
	//Uniquement de la part des voisins connus ET symétriques
	if (TVA.find(from) == TVA.end())
		return;
	else if (!TVA[from].symmetrical)
		return;

	TLVData data = dataTLV.content.data;
	int time = GetTime();

	//L'a t-on déjà reçu?
	DATAID id;
	copyUUID(data.senderID, id.id);
	id.nonce = data.nonce;
	if (RR.find(id) == RR.end())
	{
		//Pas encore reçu 
		if (equalsUUID(data.senderID, myId))
		{
			//Un de nos messages qui revient, on l'ACK.
			pushTLVToSend(tlvAck(dataTLV), from);
			return;
		}

		if (RR.size() > MAX_RR)//Taille limitée, on ne peut pas le recevoir
			return;
		//Dans tous les cas, un ACK
		pushTLVToSend(tlvAck(dataTLV), from);

		//Pas encore reçu : on l'affiche
		writeLine(data.data);

		//Puis on l'ajoute
		DATAINFO info;
		info.receptionTime = time;
		info.tlv = dataTLV;
		for (auto& entry : TVA)
		{
			//À inonder vers chaque voisin actif, sauf l'emetteur
			if (entry.first != from)
			{
				info.toFlood[entry.first] = { 0,time + RandomInt(0,1000) };
			}
		}
		RR[id] = info;
		DEBUG("Ajouté à RR le message suivant : " + tlvToString(dataTLV));
		for (auto& e : RR[id].toFlood)
		{
			DEBUG("toFlood : " + to_string(e.second.first) + ", " + to_string(e.second.second));
		}
	}
	else
	{
		//Déjà reçu, on le considère comme un ACK venant de ce pair pour cette donnée.
		DATAID i;
		copyUUID(data.senderID, i.id);
		i.nonce = data.nonce;

		DEBUG("Suppression d'un TLV pour un voisin particulier, sur réception d'un data reçu de celui-ci (" + to_string(from.addrIP[15]) + ") : " + tlvToString(dataTLV));
		DEBUGHEX(data.data, data.dataLen);
		RR[i].toFlood.erase(from);

		//Dans tous les cas, un ACK
		pushTLVToSend(tlvAck(dataTLV), from);

		freeTLV(dataTLV);
	}
}

void Table_ACKFrom(TLV& ackTLV, ADDRESS& from)
{
	TLVAck ack = ackTLV.content.ack;
	DATAID i;
	copyUUID(ack.senderID, i.id);
	i.nonce = ack.nonce;
	if (RR.find(i) != RR.end())
	{
		RR[i].toFlood.erase(from);
	}
	freeTLV(ackTLV);
}

void Table_CleanRR()
{
	list<DATAID> toRemove;
	int time = GetTime();
	for (auto& entry : RR)
	{
		if (time - entry.second.receptionTime > DATA_LIFETIME)
			toRemove.push_back(entry.first);
	}

	for (DATAID i : toRemove)
	{
		freeTLV(RR[i].tlv);
		RR.erase(i);
	}
}

void Table_RefreshTVA()
{
	static int lastHelloSent = -HELLO_DELAY;
	static int lastLongHelloSent = 0;
	int time = GetTime();
	list<ADDRESS> toRemove;
	list<ADDRESS> nonSym;
	for (auto& entry : TVA)
	{
		if (time - entry.second.lastHelloDate > 2 * MINUTE)
		{
			//Supprimé de cette table si reçu aucun hello depuis deux minutes
			toRemove.push_back(entry.first);
			string message = "Inactif depuis 2 minutes";
			//Code 2
			pushTLVToSend(tlvGoAway(TLV_GOAWAY_IDLE, message.size(), message.c_str()), entry.first);
			DEBUG("TVA : Envoyé goaway code 2 car plus de hello depuis 2 minutes de " + UUIDtoString(entry.second.id));
			continue;
		}
		else if (time - entry.second.lastHello16Date > 2 * MINUTE)
		{
			//Non symétrique si pas reçu de long hello depuis deux minutes
			entry.second.symmetrical = false;
			DEBUG("TVA : Voisin marqué non symétrique : " + UUIDtoString(entry.second.id));
		}
		if (!entry.second.symmetrical)
			nonSym.push_back(entry.first);
	}

	//Nettoyage
	for (ADDRESS tr : toRemove)
	{
		eraseFromTVA(tr);
	}
	toRemove.clear();

	//Pas assez de voisins symétriques
	if (TVA.size() - nonSym.size() < minSymNeighbours)
	{
		if (time - lastHelloSent > HELLO_DELAY)
		{
			//toutes les 15 secondes, on envoie un hello court aux voisins non symétriques aux TVP
			/*for (ADDRESS add : nonSym)
				pushTLVToSend(tlvHello(myId), add);*/
			sendHello(nonSym);//MAJ : en multicast
			lastHelloSent = time;
		}
	}

	if (time - lastLongHelloSent > HELLO_LONG_DELAY)
	{
		//Toutes les 30sec, envoi d'un Hello long à tous les voisins, symétriques ou pas.
		for (auto& entry : TVA)
		{
			INFOPAIR& p = entry.second;
			pushTLVToSend(tlvHello(myId, p.id), entry.first);
		}
		lastLongHelloSent = time;
	}
}

void Flood()
{
	int time = GetTime();

	for (auto& entry : RR)
	{
		const DATAID& id = entry.first;
		DATAINFO& info = entry.second;

		for (auto& dest : info.toFlood)
		{
			if (dest.second.first >= 5)
			{
				//Mort ou très lent, envoi de GoAway de code 2
				pushTLVToSend(tlvGoAway(TLV_GOAWAY_IDLE, 11, "Très lent"));
				//Retirer de la table des actifs
				eraseFromTVA(dest.first);

				DEBUG("Les données ont été envoyées plus de 5 fois");
			}
			else
			{
				//Moins de 5 réémissions, on regarde s'il est temps de l'envoyer
				//dest.second = paire (nombre de réémissions, instant d'envoi prévu)
				if (time >= dest.second.second)
				{

					//On l'envoie
					pushTLVToSend(info.tlv, dest.first);

					dest.second = { dest.second.first + 1,
						time + RandomInt(//entre 2^n et 2^{n+1}
							1000 * (1 << (dest.second.first)),
							1000 * (1 << (dest.second.first + 1))) };
					DEBUG("Envoi numéro " + to_string(dest.second.first) + " de " + tlvToString(info.tlv) + " prochain envoi : " + to_string(dest.second.second));
				}
			}
		}
	}
}

void sendHello(list<ADDRESS>& nonSym)
{
	if (multifd > 0)
	{
		TLV h = tlvHello(myId);
		int count = tlvLen(h);
		char* buf = new char[count + 4];
		buf[0] = MIRC_MAGIC;
		buf[1] = 0;
		encodeTLV(h, buf + 4);
		unsigned short ll = ShortToNetwork(count);
		memcpy(buf + 2, (char*)(&ll), 2);
		socklen_t l = sizeof(multiaddr);
		sendto(fd, buf, count + 4, 0, (struct sockaddr*)(&multiaddr), l);
		freeTLV(h);
		delete[] buf;
	}
	else
	{
		//Pas de multicast, envoi aux non symétriques
		for (ADDRESS add : nonSym)
			pushTLVToSend(tlvHello(myId), add);
	}
}

void eraseFromTVA(const ADDRESS& addr)
{
	TVA.erase(addr);
	//On enlève aussi les clés des gens à inonder de RR
	for (auto& entry : RR)
	{
		DATAINFO info = entry.second;
		info.toFlood.erase(addr);
	}
	//Ainsi que des tableaux toSend
	eraseFromSendList(addr);
}

void freeAllTables()
{
#ifdef VERBOSE
	printTables();
#endif
	for (auto& entry : RR)
	{
		freeTLV(entry.second.tlv);
		entry.second.toFlood.clear();
	}
	TVA.clear();
	TVP.clear();
}

#ifdef VERBOSE
void printTables()
{
	DEBUG("Statistiques : \nDonnées à inonder (RR) = " + to_string(RR.size()) + "\n"
		+ "TVA = " + to_string(TVA.size()) + "  TVP = " + to_string(TVP.size()));

	char* dest = new char[50]{ 0 };
	for (auto& entry : RR)
	{
		cout << tlvToString(entry.second.tlv) << endl;
		cout << "infos{\n";
		for (auto& entry2 : entry.second.toFlood)
		{
			inet_ntop(AF_INET6, &entry2.first.nativeAddr.sin6_addr, dest, 50);
			cout << "à dest. de " << dest << " " << entry2.second.first << " fois, prochain envoi à " << entry2.second.second << endl;

		}
		cout << "}\n";
	}
	cout << "-------TVA-------\n";
	for (auto& entry : TVA)
	{
		inet_ntop(AF_INET6, &entry.first.nativeAddr.sin6_addr, dest, 50);
		cout << "Voisin actif " << dest << (entry.second.symmetrical ? " Sym " : " ") << endl;
	}
	cout << "-------TVP-------\n";
	for (auto& entry : TVP)
	{
		inet_ntop(AF_INET6, &entry.first.nativeAddr.sin6_addr, dest, 50);
		cout << "Voisin potentiel " << dest << " d'Id " << UUIDtoString(entry.second) << endl;
	}
	delete[] dest;
}
#else
void printTables()
{
	writeLine("Statistiques : \nDonnées à inonder (RR) = " + to_string(RR.size()) + "\n"
		+ "TVA = " + to_string(TVA.size()) + "  TVP = " + to_string(TVP.size()));

	char* dest = new char[50]{ 0 };
	for (auto& entry : RR)
	{
		writeLine(tlvToString(entry.second.tlv));
		writeLine("infos{");
		for (auto& entry2 : entry.second.toFlood)
		{
			inet_ntop(AF_INET6, &entry2.first.nativeAddr.sin6_addr, dest, 50);
			writeLine("à dest. de " + string(dest) + " " + to_string(entry2.second.first) + " fois, prochain envoi à " + to_string(entry2.second.second));

		}
		writeLine("}\n");
	}
	writeLine("-------TVA--------");
	for (auto& entry : TVA)
	{
		inet_ntop(AF_INET6, &entry.first.nativeAddr.sin6_addr, dest, 50);
		writeLine("Voisin actif " + string(dest) + (entry.second.symmetrical ? " Sym " : " "));
	}
	writeLine("-------TVP--------");
	for (auto& entry : TVP)
	{
		inet_ntop(AF_INET6, &entry.first.nativeAddr.sin6_addr, dest, 50);
		writeLine("Voisin potentiel " + string(dest) + " d'Id " + UUIDtoString(entry.second));
	}
	delete[] dest;

	writeLine("Activez le mode verbeux pour pouvoir défiler.");
}
#endif