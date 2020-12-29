#include "tlv.h"

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