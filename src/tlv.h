#ifndef TLV_H
#define TLV_H

#include <map>
#include <list>
#include <string.h>
#include <string>

using namespace std;

/*Types de TLV*/
#define TLV_PAD1			0
#define TLV_PADN			1
#define TLV_HELLO			2
#define TLV_NEIGHBOUR		3
#define TLV_DATA			4
#define TLV_ACK				5
#define TLV_GOAWAY			6
#define TLV_WARNING			7
#define TLV_FILE			0xf1 //TODO
#define TLV_MaxType			7

#define TLV_MAXSIZE			257
typedef char UUID[8];

string UUIDtoString(UUID u);

bool equalsUUID(const UUID a,const  UUID b);

void copyUUID(UUID from, UUID to);

using namespace std;

typedef struct
{
	UUID sourceID;
	bool longFormat;
	UUID destID;
} TLVHello;

typedef struct
{
	char addrIP[16];
	unsigned short port;
} TLVNeighbour;

typedef struct
{
	UUID senderID;
	unsigned int  nonce;
	int dataLen;
	char* data;
} TLVData;

typedef struct
{
	UUID senderID;
	unsigned int  nonce;
} TLVAck;

typedef struct
{
	char code;
	unsigned char messageLength;
	char* message;
} TLVGoAway;

typedef struct
{
	unsigned char length;
	char* message;
} TLVWarning;

typedef struct
{
	unsigned char len;
	char* MBZ;
} TLVPadN;

typedef union
{
	TLVPadN padN;
	TLVHello hello;
	TLVNeighbour neighbour;
	TLVData data;
	TLVAck ack;
	TLVGoAway goAway;
	TLVWarning warning;
} TLV_data;

typedef struct TLV_s
{
	char type;
	TLV_data content;
	TLV_s(char t);
	TLV_s();
} TLV;

/// <summary>
/// Type -> TLV
/// </summary>
typedef map<int, list<TLV>> MIRC_DGRAM;

MIRC_DGRAM dgramInit();

#endif

/// <summary>
/// Supprime les éventuels emplacements de mémoire réservés ailleurs que sur la pile
/// (les données de taille variable notamment).
/// Ne supprime pas le TLV en tant que structure. S'il a été alloué sur le tas avec new/malloc, il devra
/// être supprimé avec delete/free ensuite.
/// </summary>
/// <param name="tlv"></param>
void freeTLV(TLV& tlv);

string tlvToString(TLV& tlv);