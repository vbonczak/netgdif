#ifndef MIRC_H
#define MIRC_H

#include "utils.h"
#include "tlv.h"

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

char* mircstrerror(int code);

int parseDatagram(char* data, unsigned int length, MIRC_DGRAM& content);

int parseDatagram0(char* data, unsigned int length, MIRC_DGRAM& content);

int parseTLVCollection(char* data, unsigned int sz, MIRC_DGRAM& content);

/*
Les fonctions parseTLV_* prennent en paramètre les données décalées de 1 (pour
ne pas inclure le type, déjà traité). le champ body contient donc le contenu
du message brut (et le reste des données du datagramme).
Elles renseignent également la taille consommée par le TLV.
*/
int parseTLV_Hello(char* body, unsigned int sz, unsigned int* parsedLength, TLV* );
int parseTLV_Neighbour(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_Data(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_ACK(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_PADN(char* body, unsigned int sz, unsigned int* parsedLength);
int parseTLV_GoAway(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);
int parseTLV_Warning(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv);

#endif
