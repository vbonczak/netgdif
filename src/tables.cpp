#include "tables.h"
#include "mirc.h"

unordered_map<ADDRESS, UUID, ADDRESSHash> TVP;

unordered_map<ADDRESS, INFOPAIR, ADDRESSHash> TVA;

unordered_map<DATAID, DATAINFO, DATAIDHash> RR;

int MAX_RR = 100;

void Table_HelloFrom(ADDRESS& addr, TLV* helloTLV)
{
	if (TVP.find(addr) != TVP.end())
	{
		//On identifie l'ID de notre voisin potentiel reçu de NEIGHBOUR précédemment
		copyUUID(helloTLV->content.hello.sourceID, TVP[addr]);
	}

	int time = GetTime();
	if (TVA.find(addr) == TVA.end())
	{
		//Pas encore connu comme voisin actif
		TVA[addr] = INFOPAIR();
		copyUUID(helloTLV->content.hello.sourceID, TVA[addr].id);
		if (helloTLV->content.hello.longFormat)
		{
			if (equalsUUID(myId, helloTLV->content.hello.destID))
			{
				//Symétrique
				TVA[addr].symmetrical = true;
			}
		}
		else
			TVA[addr].lastHello16Date = 0;//Au début, jamais reçu de long
	}
	//Mise à jour des instants de réception des hellos.
	TVA[addr].lastHelloDate = time;
	if (helloTLV->content.hello.longFormat && equalsUUID(myId, helloTLV->content.hello.destID))
	{
		TVA[addr].lastHello16Date = time;
	}
}

void Table_DataFrom(TLV* dataTLV, ADDRESS& from)
{
	TLVData data = dataTLV->content.data;
	int time = GetTime();
	//L'a t-on déjà reçu?
	DATAID id;
	copyUUID(data.senderID, id.id);
	id.nonce = data.nonce;
	if (RR.find(id) != RR.end())
	{
		//Pas encore reçu 
		if (RR.size() > MAX_RR)//Taille limitée, on ne peut pas le recevoir
			return;
		//Dans tous les cas, un ACK
		pushTLVToSend(tlvAck(dataTLV), from);
		//Pas encore reçu : on l'affiche
		data.data[data.dataLen - 1] = '\0';
		cout << data.data << endl;

		//Puis on l'ajoute
		DATAINFO info;
		info.tlv = dataTLV;
		for (auto& entry : TVA)
		{
			//À inonder vers chaque voisin actif, sauf l'emetteur
			if (entry.first != from)
				info.toFlood[entry.first] = { 0,time + RandomInt(0,1000) };
		}
		RR[id] = info;
	}
	else
	{
		//Déjà reçu, on le considère comme un ACK venant de ce pair pour cette donnée.
		DATAID i;
		copyUUID(data.senderID, i.id);
		i.nonce = data.nonce;
		RR[i].toFlood.erase(from);

		//Dans tous les cas, un ACK
		pushTLVToSend(tlvAck(dataTLV), from);
	}
}

void Table_ACKFrom(TLV* ackTLV, ADDRESS& from)
{
	TLVAck ack = ackTLV->content.ack;
	DATAID i;
	copyUUID(ack.senderID, i.id);
	i.nonce = ack.nonce;
	RR[i].toFlood.erase(from);
}

void Table_RefreshTVA()
{
	static int lastHelloSent = 0;
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
			DEBUG("TVA : Envoyé goaway code 2");
			continue;
		}
		else if (time - entry.second.lastHello16Date > 2 * MINUTE)
		{
			//Non symétrique si pas reçu de long hello depuis deux minutes
			entry.second.symmetrical = false;
			DEBUG("TVA : Voisin marqué non symétrique");
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
		if (time - lastHelloSent > 15000)
		{
			//toutes les 15 secondes, on envoie un hello court aux voisins non symétriques
			for (ADDRESS add : nonSym)
				pushTLVToSend(tlvHello(myId), add);
			lastHelloSent = time;
		}
	}

	if (time - lastLongHelloSent > 30000)
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
		DATAID id = entry.first;
		DATAINFO info = entry.second;

		for (auto& dest : info.toFlood)
		{
			if (dest.second.first >= 5)
			{
				//Mort ou très lent, envoi de GoAway de code 2
				pushTLVToSend(tlvGoAway(TLV_GOAWAY_IDLE, 10, "Très lent"));
				//Retirer de la table des actifs
				eraseFromTVA(dest.first);
			}
			else
			{
				//Moins de 5 réémissions, on regarde s'il est temps de l'envoyer
				//dest.second = paire (nombre de réémissions, instant d'envoi prévu)
				if (time >= dest.second.second)
				{
					pushTLVToSend(info.tlv, dest.first);
					info.toFlood[dest.first] = { dest.second.first + 1,
						time + RandomInt(
							1000 * (1 << (dest.second.first - 1)),
							1000 * (1 << dest.second.first)) };
				}
			}
		}
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