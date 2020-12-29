#include "tlv.h"

MIRC_DGRAM dgramInit()
{
	MIRC_DGRAM ret;

	for (int i = TLV_HELLO; i <= TLV_MaxType; i++)
	{
		ret[i] = list<TLV>();
	}

	return ret;
}

void freeTLV(int type, TLV* tlv)
{
	switch (type)
	{ 
	case TLV_DATA:
		delete tlv->data.data;
		break;
	case TLV_GOAWAY:
		delete tlv->goAway.message;
		break;
	case TLV_WARNING:
		delete tlv->warning.message;
		break;
	default:
		//Pas de bloc variable
		break;
	}
}