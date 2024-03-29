#include "tlv.h"
#include "utils.h"

string UUIDtoString(UUID u)
{
	return to_string(hash<long>()(*(long*)u));
}

bool equalsUUID(const UUID a, const UUID b)
{
	return tabEq(a, b, 8);
}

void copyUUID(UUID from, UUID to)
{
	memcpy(to, from, 8);
}

MIRC_DGRAM dgramInit()
{
	MIRC_DGRAM ret;

	for (int i = TLV_HELLO; i <= TLV_MaxType; i++)
	{
		ret[i];
	}

	//ret[TLV_FILE]; The future

	return ret;
}

void freeTLV(TLV& tlv)
{
	switch (tlv.type)
	{
	case TLV_PADN:
		delete[] tlv.content.padN.MBZ;
		break;
	case TLV_DATA:
		delete[] tlv.content.data.data;
		break;
	case TLV_GOAWAY:
		delete[] tlv.content.goAway.message;
		break;
	case TLV_WARNING:
		delete[] tlv.content.warning.message;
		break;
	default:
		//Pas de bloc variable allou� sur le tas.
		break;
	}
}

string tlvToString(TLV& tlv)
{
	string ret = "";
	char type = tlv.type;
	TLV_data dt = tlv.content;
	struct sockaddr_in6 r;
	char* dest;
	switch (type)
	{
	case TLV_PADN:				/*Ignor� de m�me*/
		ret += "PadN(" + to_string(dt.padN.len) + ")";
		break;
	case TLV_HELLO:
		ret += "TLV_HELLO de " + UUIDtoString(dt.hello.sourceID) + "  " + (dt.hello.longFormat ? "#" + UUIDtoString(dt.hello.destID) + "#" : "(court)");
		break;
	case TLV_NEIGHBOUR:
		dest = new char[50]{ 0 };
		r = address2IP(dt.neighbour.addrIP, dt.neighbour.port);
		inet_ntop(AF_INET6, &r.sin6_addr, dest, 50);
		ret += "TLV_NEIGHBOUR indiquant " + string(dest);
		delete[] dest;
		break;
	case TLV_DATA:
		dt.data.data[dt.data.dataLen - 1] = 0;
		ret += "TLV_DATA " + to_string(dt.data.nonce) + " de " + UUIDtoString(dt.data.senderID) + " : " + string(dt.data.data);
		break;
	case TLV_ACK:
		ret += "TLV_ACK  " + to_string(dt.ack.nonce) + " de " + UUIDtoString(dt.ack.senderID);
		break;
	case TLV_GOAWAY:
		ret += "TLV_GOAWAY " + to_string(dt.goAway.code) + " : " + string(dt.goAway.message);
		break;
	case TLV_WARNING:
		ret += "TLV_WARNING  : " + string(dt.warning.message);
		break;
	default:
		ret += "Inconnu(" + to_string(type) + ")";
		break;
	}

	return ret;
}

TLV_s::TLV_s(char t)
{
	type = t;
}

TLV_s::TLV_s()
{

}
