#include "mirc.h"

UUID myId;

//TLVs à envoyer pour chaque voisin
unordered_map<ADDRESS, list<TLV>, ADDRESSHash> toSend;

list<TLV> pendingForFlood; //liste accedée par les deux threads, sujette à mutex
mutex pendingForFloodLock;

//Nombre minimal de voisins symétriques
int minSymNeighbours = 10;

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
	unsigned short bodyLength = ShortFromNetwork(*((unsigned short*)data));

	if (length > bodyLength + 2)
	{
		return PARSE_ETOOBIG;
	}
	else
	{
		return parseTLVCollection(data + 2, bodyLength, content);
	}
}

int parseTLVCollection(char* body, unsigned short sz, MIRC_DGRAM& content)
{
	DEBUG(string(__PRETTY_FUNCTION__));

	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned int length = 0;
	int ret = 0;
	while (sz > 0)
	{
		TLV_data cur;
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
			ret = parseTLV_Hello(body + 1, sz - 1, &length, cur);
			break;
		case TLV_NEIGHBOUR:
			ret = parseTLV_Neighbour(body + 1, sz - 1, &length, cur);
			break;
		case TLV_DATA:
			ret = parseTLV_Data(body + 1, sz - 1, &length, cur);
			break;
		case TLV_ACK:
			ret = parseTLV_ACK(body + 1, sz - 1, &length, cur);
			break;
		case TLV_GOAWAY:
			ret = parseTLV_GoAway(body + 1, sz - 1, &length, cur);
			break;
		case TLV_WARNING:
			ret = parseTLV_Warning(body + 1, sz - 1, &length, cur);
			break;
		default:
			length = body[1];
			DEBUG("Type de TLV ignoré : " + to_string(type) + " de longueur " + to_string(length));
			DEBUGHEX(body, sz);
			exit(0);
			break;
		}

		//Chaque TLV sauf PAD1 est construit sur le schéma TYPE LONGUEUR (deux octets)
		if (type != TLV_PAD1)
			length += 2;

		body += length;
		sz -= length;
		if (ret == 0 && type != TLV_PADN)
		{
			//ajout à la liste des TLV 
			TLV curtlv(type);
			curtlv.content = cur;
			content[type].push_back(curtlv);
			cout << "           REÇU" << tlvToString(curtlv) << endl;
		}
		else
			DEBUG(mircstrerror(ret));
	}

}

int parseTLV_PADN(char* body, unsigned int sz, unsigned int* parsedLength)
{
	DEBUG(string(__PRETTY_FUNCTION__));
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

int parseTLV_Hello(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	if (sz < 9) /*Au moins l'octet de la longueur et le format HELLO court*/
		return PARSE_EEMPTYTLV;

	if (body[0] == 8) /*Format court*/
	{
		memcpy(tlv.hello.sourceID, body + 1, 8);
		tlv.hello.longFormat = false;
	}
	else if (body[0] == 16)
	{
		memcpy(tlv.hello.sourceID, body + 1, 8);
		memcpy(tlv.hello.destID, body + 9, 8);
		tlv.hello.longFormat = true;

	}
	else
		return PARSE_EINVALID;

	*parsedLength = body[0];

	return 0;
}

int parseTLV_Neighbour(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	if (body[0] != 18)/*Taille fixe de 18 octets : longueur puis 128 bits d'IPv6 (ou v4-mapped) puis 2 octets de port*/
		return PARSE_EINVALID;

	unsigned short port = ShortFromNetwork(*((unsigned short*)(body + 17)));
	tlv.neighbour.port = port;
	/*l'IP se situe de 1 à 16 inclus*/
	memcpy(tlv.neighbour.addrIP, body + 1, 16);
	return 0;
}

int parseTLV_Data(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 12) || (len + 1 > sz))
		return PARSE_EINVALID;

	memcpy(tlv.data.senderID, body + 1, 8);
	unsigned int nonce = IntFromNetwork(*(unsigned int*)(body + 9));
	tlv.data.nonce = nonce;
	tlv.data.dataLen = len - 12;
	tlv.data.data = new char[len - 12];
	memcpy(tlv.data.data, body + 13, len - 12);

	return 0;
}

int parseTLV_ACK(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	char len = body[0];

	if ((len < 12) || (len + 1 > sz))
		return PARSE_EINVALID;

	memcpy(tlv.ack.senderID, body + 1, 8);
	unsigned int nonce = IntFromNetwork(*(unsigned int*)(body + 9));

	tlv.ack.nonce = nonce;

	return 0;
}

int parseTLV_GoAway(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned char len = body[0];

	if ((len < 1) || (len + 1 > sz))
		return PARSE_EINVALID;

	tlv.goAway.code = body[1];
	tlv.goAway.message = new char[len - 1];
	memcpy(tlv.goAway.message, body + 2, len - 1);
	tlv.goAway.messageLength = len - 1;

	return 0;
}

int parseTLV_Warning(char* body, unsigned int sz, unsigned int* parsedLength, TLV_data& tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	if (sz < 1)
		return PARSE_EEMPTYTLV;

	unsigned char len = body[0];

	if (len + 1 > sz)
		return PARSE_ETOOBIG;

	tlv.warning.message = new char[len];
	memcpy(tlv.warning.message, body + 1, len);
	tlv.warning.length = len;
	return 0;
}

TLV tlvHello(UUID sender)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_HELLO);
	copyUUID(sender, ret.content.hello.sourceID);
	ret.content.hello.longFormat = false;
	return ret;
}

TLV tlvHello(UUID sender, UUID dest)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_HELLO);
	copyUUID(sender, ret.content.hello.sourceID);
	ret.content.hello.longFormat = true;
	copyUUID(dest, ret.content.hello.destID);
	return ret;
}

TLV tlvNeighbour(const char addrIP[16], unsigned short port)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_NEIGHBOUR);
	memcpy(ret.content.neighbour.addrIP, addrIP, 16);
	ret.content.neighbour.port = port;
	return ret;
}

TLV tlvData(UUID senderID, const char* data, int length, int nonce)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_DATA);
	ret.content.data.data = new char[length];
	memcpy(ret.content.data.data, data, length);
	ret.content.data.dataLen = length;
	ret.content.data.nonce = nonce; //*((unsigned int*)RandomBytes(sizeof(int)));
	copyUUID(senderID, ret.content.data.senderID);
	return ret;
}

TLV tlvAck(TLV& data)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_ACK);
	ret.content.ack.nonce = data.content.data.nonce;
	copyUUID(data.content.data.senderID, ret.content.ack.senderID);
	return ret;
}

TLV tlvPadN(unsigned char len)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_PADN);
	ret.content.padN.len = len;
	ret.content.padN.MBZ = new char[len];
	for (int i = 0; i < len; i++)
	{
		ret.content.padN.MBZ[i] = 0;
	}
	return ret;
}

TLV tlvGoAway(char code, unsigned char messageLength, const char* message)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_GOAWAY);
	ret.content.goAway.code = code;
	ret.content.goAway.message = new char[messageLength];
	memcpy(ret.content.goAway.message, message, messageLength);
	ret.content.goAway.messageLength = messageLength;
	return ret;
}

TLV tlvWarning(unsigned char length, const char* message)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	TLV ret(TLV_WARNING);
	ret.content.warning.message = new char[length];
	memcpy(ret.content.warning.message, message, length);
	ret.content.warning.length = length;
	return ret;
}

void pushTLVDATAToFlood(TLV tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	pendingForFloodLock.lock();
	pendingForFlood.push_back(tlv);
	pendingForFloodLock.unlock();
}

void pushPendingForFlood()
{
	DEBUG(string(__PRETTY_FUNCTION__));
	pendingForFloodLock.lock();
	for (TLV tlv : pendingForFlood)
	{
		TLVData data = tlv.content.data;
		DATAID id;

		copyUUID(data.senderID, id.id);
		id.nonce = data.nonce;
		for (auto& voisins : TVA)
		{
			RR[id].tlv = tlv;
			RR[id].receptionTime = GetTime();
			RR[id].toFlood[voisins.first] = { 0, GetTime() + RandomInt(0,1000) };
		}
	}
	pendingForFlood.clear();
	pendingForFloodLock.unlock();
}

void pushTLVToSend(TLV tlv, const ADDRESS& dest)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	toSend[dest].push_back(tlv);
}

void pushTLVToSend(TLV tlv)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	for (auto& entry : TVA)
	{
		pushTLVToSend(tlv, entry.first);
	}
}

void sendPendingTLVs(int fd, const ADDRESS& address)
{
	DEBUG(string(__PRETTY_FUNCTION__));
	Flood();
	list<TLV>& listSend = toSend[address];

	//Paquet envoyé
	char buf[1024] = { MIRC_MAGIC,0 };
	//Les TLV en cours
	char data[1024];
	int totalLength = 4;//entête MIRC
	//Longueur du TLV en cours
	int len = 0;
	while (totalLength < 1024 && !listSend.empty())
	{
		TLV cur = listSend.front();

		len = encodeTLV(cur, data);
		if (totalLength + len > 1024)
		{
			//Trop gros pour UDP
			break;
		}
		memcpy(buf + totalLength, data, len);
		totalLength += len;
		listSend.pop_front();
	}

	switch (1024 - totalLength)
	{
	case 0:
		//Pas de problème
		break;
	case 1:
		//pad1 pour combler
		buf[1024 - 1] = 0;
		break;
	default:
		//padN pour combler
		len = encodeTLV(tlvPadN(1024 - 2 - totalLength), data);
		memcpy(buf + totalLength, data, len);
		break;
	}

	//On n'envoie pas de paquet uniquement de remplissage.
	if (totalLength > 0)
	{
		unsigned short len = ShortToNetwork(totalLength - 4);
		memcpy(buf + 2, (char*)&len, 2);
		socklen_t l = sizeof(address.nativeAddr);
		sendto(fd, buf, 1024, 0, (struct sockaddr*)(&address.nativeAddr), l);
		char* dest = new char[50]{ 0 };
		inet_ntop(AF_INET6, &address.nativeAddr.sin6_addr, dest, 50);

		DEBUG("Envoi effectif de " + to_string(totalLength - 4) + " octets à " + string(dest) + ": ");
		DEBUGHEX(buf, 1024);
		delete dest;
	}
}


int tlvLen(TLV& t)
{
	TLV_data* tlv = &(t.content);
	char type = t.type;
	int length = 0;

	switch (type)
	{
	case TLV_PAD1:
		length = 1;
		break;
	case TLV_PADN:
		length = 2 + tlv->padN.len;
		break;
	case TLV_HELLO:
		if (tlv->hello.longFormat)
		{
			length = 18;
		}
		else
		{
			length = 10;
		}
		break;
	case TLV_NEIGHBOUR:
		length = 20;
		break;
	case TLV_DATA:
		length = 14 + tlv->data.dataLen;
		break;
	case TLV_ACK:
		length = 14;
		break;
	case TLV_GOAWAY:
		length = 3 + tlv->goAway.messageLength;
		break;
	case TLV_WARNING:
		length = 2 + tlv->warning.length;
		break;
	default:
		length = 0;
		break;
	}

	return length;
}

int encodeTLV(TLV t, char* outData)
{
	TLV_data* tlv = &(t.content);
	char type = t.type;
	int length = 0;
	unsigned short curS;
	unsigned int curI;
	switch (type)
	{
	case TLV_PAD1:
		length = 1;
		break;
	case TLV_PADN:
		length = 2 + tlv->padN.len;
		memcpy(outData + 2, tlv->padN.MBZ, tlv->padN.len);
		outData[1] = (char)tlv->padN.len;
		break;
	case TLV_HELLO:
		if (tlv->hello.longFormat)
		{
			length = 18;
			outData[1] = 16;
			memcpy(outData + 2, tlv->hello.sourceID, 8);
			memcpy(outData + 10, tlv->hello.destID, 8);
		}
		else
		{
			length = 10;
			outData[1] = 8;
			memcpy(outData + 2, tlv->hello.sourceID, 8);
		}
		break;
	case TLV_NEIGHBOUR:
		length = 20;
		outData[1] = 18;
		memcpy(outData + 2, tlv->neighbour.addrIP, 16);
		curS = ShortToNetwork(tlv->neighbour.port);
		memcpy(outData + 18, (char*)(&curS), 2);
		break;
	case TLV_DATA:
		length = 14 + tlv->data.dataLen;
		outData[1] = 12 + tlv->data.dataLen;
		memcpy(outData + 2, tlv->data.senderID, 8);
		curI = IntToNetwork(tlv->data.nonce);
		memcpy(outData + 10, (char*)(&curI), 4);
		memcpy(outData + 14, tlv->data.data, tlv->data.dataLen);
		break;
	case TLV_ACK:
		length = 14;
		outData[1] = 12;
		memcpy(outData + 2, tlv->ack.senderID, 8);
		curI = IntToNetwork(tlv->data.nonce);
		memcpy(outData + 10, (char*)(&curI), 4);
		break;
	case TLV_GOAWAY:
		length = 3 + tlv->goAway.messageLength;
		outData[1] = 1 + tlv->goAway.messageLength;
		outData[2] = tlv->goAway.code;
		memcpy(outData + 3, tlv->goAway.message, tlv->goAway.messageLength);
		break;
	case TLV_WARNING:
		length = 2 + tlv->warning.length;
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

void eraseFromSendList(const ADDRESS& a)
{
	toSend.erase(a);
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

void mircQuit()
{
	pendingForFloodLock.lock();
	for (TLV t : pendingForFlood)
		freeTLV(t);
	pendingForFlood.clear();
	pendingForFloodLock.unlock();

	for (auto& entry : toSend)
	{
		for (TLV t : entry.second)
			freeTLV(t);
	}
}