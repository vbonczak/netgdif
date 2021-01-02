#ifndef MIRC_H
#define MIRC_H

#include "utils.h"
#include "tlv.h"

extern UUID myId;

/*Définitions du protocole*/
#define MIRC_MAGIC			95

/*Codes d'erreur*/
#define PARSE_EWRONGMAGIC	0x1
#define PARSE_EUNKNOWN		0x2
#define PARSE_EEMPTY		0x3
#define PARSE_EUNSUPPORTED	0x4
#define PARSE_ETOOBIG		0x5
#define PARSE_EEMPTYTLV		0x6
#define PARSE_EINVALID		0x7



/*GoAway raison de départ*/
#define TLV_GOAWAY_UNKNOWN		0
#define TLV_GOAWAY_QUIT			1
#define TLV_GOAWAY_IDLE			2
#define TLV_GOAWAY_VIOLATION	3

/*Assez souvent, on envoie les TLV neighbour à nos voisins*/
#define NEIGHBOUR_FLOODING_DELAY	MINUTE 

char* mircstrerror(int code);

/// <summary>
/// 
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <param name="content">Une référence vers un objet datagramme MIRC initialisé à l'aide de dgramInit().</param>
/// <returns></returns>
int parseDatagram(char* data, unsigned int length, MIRC_DGRAM& content);

/// <summary>
/// 
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <param name="content">Une référence vers un objet datagramme MIRC initialisé à l'aide de dgramInit().</param>
/// <returns></returns>
int parseDatagram0(char* data, unsigned int length, MIRC_DGRAM& content);

/// <summary>
///  
/// </summary>
/// <param name="data"></param>
/// <param name="sz"></param>
/// <param name="content">Une référence vers un objet datagramme MIRC initialisé à l'aide de dgramInit().</param>
/// <returns></returns>
int parseTLVCollection(char* data, unsigned int sz, MIRC_DGRAM& content);

/*
Les fonctions parseTLV_* prennent en paramètre les données décalées de 1 (pour
ne pas inclure le type, déjà traité). le champ body contient donc le contenu
du message brut (et le reste des données du datagramme).
Elles renseignent également la taille consommée par le TLV.
*/
int parseTLV_Hello(char* body, unsigned int sz, unsigned int* parsedLength, TLV*);
int parseTLV_Neighbour(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_Data(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_ACK(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_PADN(char* body, unsigned int sz, unsigned int* parsedLength);
int parseTLV_GoAway(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_Warning(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);



TLV* tlvHello(UUID sender);
TLV* tlvHello(UUID sender, UUID dest);

TLV* tlvNeighbour(char addrIP[16], unsigned short port);
TLV* tlvData(UUID senderID, char* data);
TLV* tlvAck(TLV* data);
TLV* tlvPadN(unsigned char len);

TLV* tlvGoAway(char code,
	unsigned char messageLength,
	const char* message);

TLV* tlvWarning(unsigned char length,
	const char* message);

/// <summary>
/// Place le TLV dans la file de données à envoyer.
/// </summary>
/// <param name="tlv"></param>
void pushTLVToSend(TLV* tlv);

/// <summary>
/// Encode le TLV dans outData (non alloué avant l'appel), et renvoie
/// la taille de ces données.
/// </summary>
/// <param name="tlv"></param>
/// <param name="outData"></param>
/// <returns></returns>
int encodeTLV(TLV* tlv, int type, char* outData);

 
#endif
