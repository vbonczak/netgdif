#include "mirc.h"

int parseDatagram(char* data, unsigned int length)
{
	if (length <= 3)
		return PARSE_EEMPTY;

	if (data[0] != MIRC_MAGIC)
		return PARSE_EWRONGMAGIC;

	switch (data[1])/*Version*/
	{
	case 0:
		return parseDatagram0(data + 2, length - 2);
		break;
	default:
		return PARSE_EUNSUPPORTED;
		break;
	}

}

int parseDatagram0(char* data, unsigned int length)
{
	unsigned int bodyLength = ShortFromNetwork(*((unsigned short*)data));
	if (length > bodyLength + 2)
	{
		return PARSE_ETOOBIG;
	}
	else
	{
		return parseTLVCollection(data + 2, bodyLength);
	}
}

int parseTLVCollection(char* body, unsigned int sz)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned int length = 0;
	int ret = 0;
	while (sz > 0)
	{
		char type = body[0];
		switch (type)
		{
		case TLV_PAD1:				/*Silencieusement ignoré*/
			length = 8;
			break;
		case TLV_PADN:				/*Ignoré de même*/
			ret = parseTLV_PADN(body + 1, sz - 1, &length);
			break;
		case TLV_HELLO:
			ret = parseTLV_Hello(body + 1, sz - 1, &length);
			break;
		case TLV_NEIGHBOUR:
			ret = parseTLV_Neighbour(body + 1, sz - 1, &length);
			break;
		case TLV_DATA:
			ret = parseTLV_Data(body + 1, sz - 1, &length);
			break;
		case TLV_ACK:
			ret = parseTLV_ACK(body + 1, sz - 1, &length);
			break;
		case TLV_GOAWAY:
			ret = parseTLV_GoAway(body + 1, sz - 1, &length);
			break;
		case TLV_WARNING:
			ret = parseTLV_Warning(body + 1, sz - 1, &length);
			break;
		default:
			DEBUG("Type de TLV ignoré");
			break;
		}
		body += length;
		sz -= length;
		if (ret == 0)
		{
			//ajout  à la liste des TLV 
		}
		else
			DEBUG(mircstrerror(ret));
	}

}

int parseTLV_PADN(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned char lgt = body[0];

	if (lgt > sz - 1)
		return PARSE_ETOOBIG;

	for (int i = 1; i < lgt; i++)
	{
		if (body[i] != 0)
			return PARSE_EINVALID;
	}

	*parsedLength = lgt;	/*Conversion sans perte*/

	return 0;
}

int parseTLV_Hello(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 9) /*Au moins l'octet de la longueur et le format HELLO court*/
		return PARSE_EEMPTYTLV;
	char sourceID[8];
	if (body[0] == 8) /*Format court*/
	{
		memcpy(sourceID, body + 1, 8);
		/*TODO faire qqch avec ça*/
	}
	else if (body[0] == 16)
	{
		memcpy(sourceID, body + 1, 8);
		char destID[8];
		memcpy(destID, body + 9, 8);
		/*TODO faire qqch avec ça*/
	}
	else
		return PARSE_EINVALID;

	*parsedLength = body[0];

	return 0;
}

int parseTLV_Neighbour(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	if (body[0] != 18)/*Taille fixe de 18 octets : longueur puis 128 bits d'IPv6 (ou v4-mapped) puis 2 octets de port*/
		return PARSE_EINVALID;

	unsigned short port = ShortFromNetwork(*((unsigned short*)(body + 17)));
	/*l'IP se situe de 1 à 16 inclus*/
	return 0;
}

int parseTLV_Data(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 12) || (len + 1 > sz))
		return PARSE_EINVALID;

	char senderID[8];
	memcpy(senderID, body + 1, 8);
	unsigned int nonce = IntFromNetwork(*(unsigned int*)(body + 9));

	char* data = new char[len - 12];
	memcpy(data, body + 13, len - 12);

	/*todo faire qqch avec ces infos*/

	return 0;
}

int parseTLV_ACK(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 12) || (len + 1 > sz))
		return PARSE_EINVALID;

	char senderID[8];
	memcpy(senderID, body + 1, 8);
	unsigned int nonce = IntFromNetwork(*(unsigned int*)(body + 9));

	/*todo faire qqch avec ces infos*/

	return 0;
}

int parseTLV_GoAway(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 1) || (len + 1 > sz))
		return PARSE_EINVALID;

	char code = body[1];
	char* message = new char[len - 1];
	memcpy(message, body + 2, len - 1);
	DEBUG(message);
	/*TODO traiter ces infos*/
	return 0;
}

int parseTLV_Warning(char* body, unsigned int sz, unsigned int* parsedLength)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if (len + 1 > sz)
		return PARSE_ETOOBIG;

	char* message = new char[len - 1];
	memcpy(message, body + 1, len);
	/*TODO traiter ces infos*/
	return 0;
}



/*Transcription des erreurs*/

char* mircstrerror(int code)
{
	switch (code)
	{
	case PARSE_EWRONGMAGIC:
		return "Signature du datagramme invalide.\n";
	case PARSE_EUNKNOWN:
		return "Version inconnue.\n";
	case PARSE_EEMPTY:
		return "Datagramme vide ou de taille incorrecte.\n";
	case PARSE_EUNSUPPORTED:
		return "Paramètre non supporté.\n";
	case PARSE_ETOOBIG:
		return "Datagramme ou TLV en dépassement de capacité.\n";
	case PARSE_EEMPTYTLV:
		return "TLV vide ou de taille trop faible.\n";
	case PARSE_EINVALID:
		return "Format incorrect.\n";
	default:
		break;
	}
}