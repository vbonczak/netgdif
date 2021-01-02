#include "tables.h"

unordered_map<ADDRESS, UUID, ADDRESSHash> TVP;

unordered_map<ADDRESS, INFOPAIR, ADDRESSHash> TVA;

unordered_map<DATAID, DATAINFO, DATAIDHash> RR;