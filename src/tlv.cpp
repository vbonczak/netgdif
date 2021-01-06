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
		ret[i] = list<TLV>();
	}

	return ret;
}

void freeTLV(TLV& tlv)
{
	switch (tlv.type)
	{
	case TLV_PADN:
		delete tlv.content.padN.MBZ;
	case TLV_DATA:
		delete tlv.content.data.data;
		break;
	case TLV_GOAWAY:
		delete tlv.content.goAway.message;
		break;
	case TLV_WARNING:
		delete tlv.content.warning.message;
		break;
	default:
		//Pas de bloc variable
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
	case TLV_PADN:				/*Ignoré de même*/
		ret += "PadN(" + to_string(dt.padN.len) + ")";
		break;
	case TLV_HELLO:
		ret += "TLV_HELLO de " + UUIDtoString(dt.hello.destID) + "  "+ (dt.hello.longFormat ? UUIDtoString(dt.hello.destID) : "(court)");
		break;
	case TLV_NEIGHBOUR:
		dest = new char[50]{ 0 };
		r = address2IP(dt.neighbour.addrIP, dt.neighbour.port);
		inet_ntop(AF_INET6, (struct sockaddr_in*)&r, dest, 50);
		ret += "TLV_NEIGHBOUR indiquant " + string(dest);
		delete dest;
		break;
	case TLV_DATA:
		ret += "TLV_DATA " + to_string(dt.data.nonce) + " de " + UUIDtoString(dt.data.senderID) + " : " + string(dt.data.data);
		break;
	case TLV_ACK:
		ret += "TLV_ACK" + to_string(dt.ack.nonce) + " de " + UUIDtoString(dt.ack.senderID);
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

	ret += '\n';
	return ret;
}

TLV_s::TLV_s(char t)
{
	type = t;
}

TLV_s::TLV_s()
{
}
