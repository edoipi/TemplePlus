#include "stdafx.h"

const uint32_t MAXLEVEL = 20;
const int CRMIN = -2; // equiv to CR 1/4  (next ones are CR 1/3, CR 1/2, CR 1, CR 2, CR 3,...
const int CRMAX = 20;
const int CRCOUNT = CRMAX - CRMIN + 1; // number of challenge ratings for our table

class XPAward{
public:
	int XPAwardTable[MAXLEVEL][CRCOUNT];
	XPAward();
};