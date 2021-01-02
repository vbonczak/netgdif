#include "tables.h"

unordered_map<ADDRESS, UUID, ADDRESSHash> TVP;

unordered_map<ADDRESS, INFOPAIR, ADDRESSHash> TVA;

unordered_map<DATAID, DATAINFO, DATAIDHash> RR;

void Table_HelloFrom(ADDRESS addr, TLV* helloTLV)
{
	if (TVP.find(addr) != TVP.end())
	{
		copyUUID(helloTLV->hello.sourceID, TVP[addr]);
	}

	int time = GetTime();
	if (TVA.find(addr) == TVA.end())
	{
		TVA[addr] = INFOPAIR();
		copyUUID(helloTLV->hello.sourceID, TVA[addr].id);
		if (helloTLV->hello.longFormat)
		{
			if (equalsUUID(myId, helloTLV->hello.destID))
			{
				//Symétrique
				TVA[addr].symmetrical = true;
			}
		}
		else
			TVA[addr].lastHello16Date = 0;
	}
	TVA[addr].lastHelloDate = time;
	if (helloTLV->hello.longFormat)
	{
		TVA[addr].lastHello16Date = time;
	}
}

void Table_RefreshTVA()
{
	int time = GetTime();
	list<ADDRESS> toRemove;
	for (auto& entry : TVA)
	{
		if (time - entry.second.lastHelloDate > 2 * MINUTE)
		{
			//Supprimé de cette table si reçu aucun hello depuis deux minutes
			toRemove.push_back(entry.first);
			string message = "Inactif depuis 2 minutes";
			//Code 2
			pushTLVToSend(tlvGoAway(TLV_GOAWAY_IDLE, message.size(), message.c_str()));
		}
		else if (time - entry.second.lastHello16Date > 2 * MINUTE)
		{
			//Non symétrique si pas reçu de long hello depuis deux minutes
			entry.second.symmetrical = false;
		}
	}
}