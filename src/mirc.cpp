#include "mirc.h"

int parseDatagram(char* data, unsigned int length, MIRC_DGRAM& content)
{
	if (length <= 3)
		return PARSE_EEMPTY;

	if (data[0] != MIRC_MAGIC)
		return PARSE_EWRONGMAGIC;

	switch (data[1])/*Version*/
	{
	case 0:
		return parseDatagram0(data + 2, length - 2, content);
		break;
	default:
		return PARSE_EUNSUPPORTED;
		break;
	}

}

int parseDatagram0(char* data, unsigned int length, MIRC_DGRAM& content)
{
	unsigned int bodyLength = ShortFromNetwork(*((unsigned short*)data));
	if (length > bodyLength + 2)
	{
		return PARSE_ETOOBIG;
	}
	else
	{
		return parseTLVCollection(data + 2, bodyLength, content);
	}
}

int parseTLVCollection(char* body, unsigned int sz, MIRC_DGRAM& content)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned int length = 0;
	int ret = 0;
	while (sz > 0)
	{
		TLV cur;
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
			ret = parseTLV_Hello(body + 1, sz - 1, &length, &cur);
			break;
		case TLV_NEIGHBOUR:
			ret = parseTLV_Neighbour(body + 1, sz - 1, &length, &cur);
			break;
		case TLV_DATA:
			ret = parseTLV_Data(body + 1, sz - 1, &length, &cur);
			break;
		case TLV_ACK:
			ret = parseTLV_ACK(body + 1, sz - 1, &length, &cur);
			break;
		case TLV_GOAWAY:
			ret = parseTLV_GoAway(body + 1, sz - 1, &length, &cur);
			break;
		case TLV_WARNING:
			ret = parseTLV_Warning(body + 1, sz - 1, &length, &cur);
			break;
		default:
			DEBUG("Type de TLV ignoré");
			break;
		}
		body += length;
		sz -= length;
		if (ret == 0)
		{
			//ajout à la liste des TLV 
			content[type].push_back(cur);
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

int parseTLV_Hello(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv)
{
	if (sz < 9) /*Au moins l'octet de la longueur et le format HELLO court*/
		return PARSE_EEMPTYTLV;
	 
	if (body[0] == 8) /*Format court*/
	{
		memcpy(tlv->hello.sourceID, body + 1, 8);
		tlv->hello.longFormat = false;
	}
	else if (body[0] == 16)
	{
		memcpy(tlv->hello.sourceID, body + 1, 8);
		memcpy(tlv->hello.destID, body + 9, 8);
		tlv->hello.longFormat = true;
		 
	}
	else
		return PARSE_EINVALID;

	*parsedLength = body[0];

	return 0;
}

int parseTLV_Neighbour(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	if (body[0] != 18)/*Taille fixe de 18 octets : longueur puis 128 bits d'IPv6 (ou v4-mapped) puis 2 octets de port*/
		return PARSE_EINVALID;

	unsigned short port = ShortFromNetwork(*((unsigned short*)(body + 17)));
	tlv->neighbour.port = port;
	/*l'IP se situe de 1 à 16 inclus*/
	memcpy(tlv->neighbour.addrIP, body + 1, 16);
	return 0;
}

int parseTLV_Data(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 12) || (len + 1 > sz))
		return PARSE_EINVALID;

	memcpy(tlv->data.senderID, body + 1, 8);
	unsigned int nonce = IntFromNetwork(*(unsigned int*)(body + 9));
	tlv->data.nonce = nonce;
	tlv->data.data = new char[len - 12];
	memcpy(tlv->data.data, body + 13, len - 12);

	return 0;
}

int parseTLV_ACK(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 12) || (len + 1 > sz))
		return PARSE_EINVALID;

	memcpy(tlv->ack.senderID, body + 1, 8);
	unsigned int nonce = IntFromNetwork(*(unsigned int*)(body + 9));

	tlv->ack.nonce = nonce;

	return 0;
}

int parseTLV_GoAway(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned char len = body[0];

	if ((len < 1) || (len + 1 > sz))
		return PARSE_EINVALID;

	tlv->goAway.code = body[1];
	memcpy(tlv->goAway.message, body + 2, len - 1);
	tlv->goAway.messageLength = len - 1;

	return 0;
}

int parseTLV_Warning(char* body, unsigned int sz, unsigned int* parsedLength, TLV* tlv)
{
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned char len = body[0];

	if (len + 1 > sz)
		return PARSE_ETOOBIG;

	memcpy(tlv->warning.message, body + 1, len);
	tlv->warning.length = len;
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