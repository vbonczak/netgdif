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
#include "mirc.h"

typedef struct ADDRESS_s
{
	char addrIP[16];
	unsigned short port;

	bool const operator==(const struct ADDRESS_s& o) const
	{
		return port == o.port && tabEq(addrIP, o.addrIP);
	}
} ADDRESS;

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
	TLV* tlv;
	/// <summary>
	/// table Id -> nombre de fois où on lui a envoyé cette donnée
	/// </summary>
	map<UUID*, int> toFlood;
} DATAINFO;

struct ADDRESSHash
{
	size_t operator()(const ADDRESS& o) const
	{
		return hash<unsigned short>()(o.port) ^ hash<uint64_t>()(*((uint64_t*)(o.addrIP))) ^ hash<uint64_t>()(*((uint64_t*)(o.addrIP + 8)));
	}
};

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
void Table_HelloFrom(ADDRESS addr, TLV* helloTLV);
#endif