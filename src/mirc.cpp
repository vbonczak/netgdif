#include "mirc.h"

UUID myId;

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
		if (ret == 0 && type != TLV_PADN)
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
	tlv->data.dataLen = len - 12;
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
	tlv->goAway.message = new char[len - 1];
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

	tlv->warning.message = new char[len];
	memcpy(tlv->warning.message, body + 1, len);
	tlv->warning.length = len;
	return 0;
}

TLV* tlvHello(UUID sender)
{
	TLV* ret = new TLV();
	copyUUID(sender, ret->hello.sourceID);
	ret->hello.longFormat = false;
	return ret;
}

TLV* tlvHello(UUID sender, UUID dest)
{
	TLV* ret = new TLV();
	copyUUID(sender, ret->hello.sourceID);
	ret->hello.longFormat = true;
	copyUUID(dest, ret->hello.destID);
	return ret;
}

TLV* tlvNeighbour(char addrIP[16], unsigned short port)
{
	TLV* ret = new TLV();
	memcpy(ret->neighbour.addrIP, addrIP, 16);
	ret->neighbour.port = port;
	return ret;
}

TLV* tlvData(UUID senderID, char* data)
{
	TLV* ret = new TLV();
	ret->data.data = data;
	ret->data.nonce = *((unsigned int*)RandomBytes(sizeof(int)));
	copyUUID(senderID, ret->data.senderID);
	return ret;
}

TLV* tlvAck(TLV* data)
{
	TLV* ret = new TLV();
	ret->ack.nonce = data->data.nonce;
	copyUUID(data->data.senderID, ret->ack.senderID);
	return ret;
}

TLV* tlvPadN(unsigned char len)
{
	TLV* ret = new TLV();
	ret->padN.len = len;
	ret->padN.MBZ = new char[len];
	for (int i = 0; i < len; i++)
	{
		ret->padN.MBZ[i] = 0;
	}
	return ret;
}

TLV* tlvGoAway(char code, unsigned char messageLength, const char* message)
{
	TLV* ret = new TLV();
	ret->goAway.code = code;
	ret->goAway.message = new char[messageLength];
	memcpy(ret->goAway.message, message, messageLength);
	ret->goAway.messageLength = messageLength;
	return ret;
}

TLV* tlvWarning(unsigned char length, const char* message)
{
	TLV* ret = new TLV();
	ret->warning.message = new char[length];
	memcpy(ret->warning.message, message, length);
	ret->warning.length = length;
	return ret;
}

void pushTLVToSend(TLV* tlv)
{
	//TODO
}

int encodeTLV(TLV* tlv, int type, char* outData)
{
	int length = 0;
	unsigned short curS;
	unsigned int curI;
	switch (type)
	{
	case TLV_PAD1:
		outData = new char[1];
		length = 1;
		break;
	case TLV_PADN:
		memcpy(outData + 2, tlv->padN.MBZ, tlv->padN.len);
		outData[1] = (char)tlv->padN.len;
		break;
	case TLV_HELLO:
		if (tlv->hello.longFormat)
		{
			outData = new char[18];
			outData[1] = 16;
			memcpy(outData + 2, tlv->hello.sourceID, 8);
			memcpy(outData + 10, tlv->hello.destID, 8);
		}
		else
		{
			outData = new char[10];
			outData[1] = 8;
			memcpy(outData + 2, tlv->hello.sourceID, 8);
		}
		break;
	case TLV_NEIGHBOUR:
		outData = new char[18];
		outData[1] = 18;
		memcpy(outData + 2, tlv->neighbour.addrIP, 16);
		curS = ShortToNetwork(tlv->neighbour.port);
		memcpy(outData + 18, (char*)(&curS), 2);
		break;
	case TLV_DATA:
		outData = new char[14 + tlv->data.dataLen];
		outData[1] = 12 + tlv->data.dataLen;
		memcpy(outData + 2, tlv->data.senderID, 8);
		curI = IntToNetwork(tlv->data.nonce);
		memcpy(outData + 10, (char*)(&curI), 4);
		memcpy(outData + 14, tlv->data.data, tlv->data.dataLen);
		break;
	case TLV_ACK:
		outData = new char[14];
		outData[1] = 12;
		memcpy(outData + 2, tlv->ack.senderID, 8);
		curI = IntToNetwork(tlv->data.nonce);
		memcpy(outData + 10, (char*)(&curI), 4);
		break;
	case TLV_GOAWAY:
		outData = new char[3 + tlv->goAway.messageLength];
		outData[1] = 1 + tlv->goAway.messageLength;
		outData[2] = tlv->goAway.code;
		memcpy(outData + 3, tlv->goAway.message, tlv->goAway.messageLength);
		break;
	case TLV_WARNING:
		outData = new char[2 + tlv->warning.length];
		outData[1] = tlv->warning.length;
		memcpy(outData + 2, tlv->warning.message, tlv->warning.length);
		break;
	default:
		DEBUG("Type de TLV ignoré à l'encodage");
		break;
	}

	outData[0] = (char)type;

	return length;
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

