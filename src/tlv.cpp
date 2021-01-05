#include "tlv.h"
#include "utils.h"

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

TLV_s::TLV_s(char t)
{
	type = t;
}

TLV_s::TLV_s()
{
}
