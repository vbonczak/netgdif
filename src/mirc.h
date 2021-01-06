#ifndef MIRC_H
#define MIRC_H

#include "utils.h"
#include "tlv.h"
#include "tables.h"
#include <iostream>
#include <mutex>

using namespace std;

extern UUID myId;
extern int minSymNeighbours;
 


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

void mircQuit();

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
int parseTLVCollection(char* data, unsigned short sz, MIRC_DGRAM& content);

/*
Les fonctions parseTLV_* prennent en paramètre les données décalées de 1 (pour
ne pas inclure le type, déjà traité). le champ body contient donc le contenu
du message brut (et le reste des données du datagramme).
Elles renseignent également la taille consommée par le TLV.
*/
int parseTLV_Hello(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data&);
int parseTLV_Neighbour(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv);
int parseTLV_Data(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv);
int parseTLV_ACK(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv);
int parseTLV_PADN(char* body, unsigned int sz, unsigned int* parsedLength);
int parseTLV_GoAway(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv);
int parseTLV_Warning(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv);

/// <summary>
/// Retourne un TLV de type Hello avec les paramètres spécifiés.
/// </summary>
/// <param name="sender"></param>
/// <returns></returns>
TLV tlvHello(UUID sender);
/// <summary>
/// Retourne un TLV de type Hello Long avec les paramètres spécifiés.
/// </summary>
/// <param name="sender"></param>
/// <param name="dest"></param>
/// <returns></returns>
TLV tlvHello(UUID sender, UUID dest);
/// <summary>
/// Retourne un TLV neighbour
/// </summary>
/// <param name="addrIP"></param>
/// <param name="port"></param>
/// <returns></returns>
TLV tlvNeighbour(const char addrIP[16], unsigned short port);
/// <summary>
/// TLV data
/// </summary>
/// <param name="senderID"></param>
/// <param name="data"></param>
/// <param name="length"></param>
/// <param name="nonce"></param>
/// <returns></returns>
TLV tlvData(UUID senderID, const char* data, int length, int nonce);
/// <summary>
/// Retourne un accusé de réception pour un TLV Data donné.
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
TLV tlvAck(TLV& data);

/// <summary>
/// Retourne un TLV de remplissage
/// </summary>
/// <param name="len"></param>
/// <returns></returns>
TLV tlvPadN(unsigned char len);

/// <summary>
/// Retourne un TLV GoAway
/// </summary>
/// <param name="code"></param>
/// <param name="messageLength"></param>
/// <param name="message"></param>
/// <returns></returns>
TLV tlvGoAway(char code,
	unsigned char messageLength,
	const char* message);

/// <summary>
/// Retourne un TLV Warning
/// </summary>
/// <param name="length"></param>
/// <param name="message"></param>
/// <returns></returns>
TLV tlvWarning(unsigned char length,
	const char* message);

/// <summary>
/// Place un TLV de données dans la file d'attente de tous les voisins.
/// </summary>
/// <param name="tlv"></param>
void pushTLVDATAToFlood(TLV tlv);

/// <summary>
/// Régulièrement appelée pour basculer les TLVs en attente en provenance du thread principal
/// vers la liste RR à inonder.
/// </summary>
void pushPendingForFlood();

/// <summary>
/// Place le TLV dans la file de données à envoyer.
/// </summary>
/// <param name="tlv"></param>
void pushTLVToSend(TLV tlv, const ADDRESS& dest);

/// <summary>
/// Place le TLV dans la file de données à envoyer à tous les voisins actifs.
/// </summary>
/// <param name="tlv"></param>
void pushTLVToSend(TLV tlv);

/// <summary>
/// Envoie les TLV réservés au destinataire spécifié en attente.
/// </summary>
/// <param name="fd"></param>
/// <param name="address"></param>
void sendPendingTLVs(int fd, const ADDRESS& address);

int tlvLen(TLV& t);

/// <summary>
/// Encode le TLV dans outData (alloué avant l'appel pour au moins 1Ko), et renvoie
/// la taille de ces données.
/// </summary>
/// <param name="t"></param>
/// <param name="outData"></param>
/// <returns></returns>
int encodeTLV(TLV t, char* outData);

void eraseFromSendList(const ADDRESS& a);
#endif
