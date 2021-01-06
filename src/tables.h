#ifndef TABLES_H
#define TABLES_H

#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <list>
#include <signal.h>
#include "tlv.h"
#include "utils.h"
 

extern int MAX_RR;



typedef struct
{
	UUID id;
	int lastHelloDate;
	int lastHello16Date;
	bool symmetrical;
} INFOPAIR;

typedef struct DATAID_s
{
	UUID id;
	unsigned int nonce;

	bool const operator==(const struct DATAID_s& o) const
	{
		return nonce == o.nonce && equalsUUID(id, o.id);
	}
} DATAID;



typedef struct
{
	TLV tlv;
	/// <summary>
	/// table Id -> (nombre de fois où on lui a envoyé cette donnée, prochain instant d'envoi en ms)
	/// </summary>
	unordered_map<ADDRESS, pair<int, int>, ADDRESSHash> toFlood;

	int receptionTime;
} DATAINFO;



struct DATAIDHash
{
	size_t operator()(const DATAID& o) const
	{
		return hash<unsigned int>()(o.nonce) ^ hash<uint64_t>()(*((uint64_t*)(o.id)));
	}
};

/// <summary>
/// Voisins potentiels
/// </summary>
extern unordered_map<ADDRESS, UUID, ADDRESSHash> TVP;

/// <summary>
/// Voisins actifs
/// </summary>
extern unordered_map<ADDRESS, INFOPAIR, ADDRESSHash> TVA;

/// <summary>
/// Les données récemment reçues (uniquement pour les TLV DATA).
/// </summary>
extern unordered_map<DATAID, DATAINFO, DATAIDHash> RR;

/// <summary>
/// 
/// </summary>
/// <param name="addr"></param>
/// <param name="helloTLV"></param>
void Table_HelloFrom(ADDRESS& addr, TLV helloTLV);
void Table_DataFrom(TLV dataTLV, ADDRESS& from);
void Table_ACKFrom(TLV& ackTLV, ADDRESS& from);

/// <summary>
/// Nettoyage des données anciennes
/// </summary>
void Table_CleanRR();

/// <summary>
/// S'occupe de savoir si les voisins sont symétriques ou pas, et si on doit envoyer
/// un Hello long (dans le cas où nous n'avons pas assez de voisins symétriques).
/// </summary>
void Table_RefreshTVA();

void Flood();


void eraseFromTVA(const ADDRESS& addr);

void freeAllTables();

/// <summary>
/// Envoi spécial d'un Hello en multicast
/// </summary>
void sendHello(list<ADDRESS>& nonSym);
#endif