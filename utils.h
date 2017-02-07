#pragma once
#include <mutex>
#include <map>
#include "ffcTypes.h"



namespace ffc {

	extern FfcMsg msgServer;
	extern bool threadActive;
	extern std::mutex mutex;

	extern int validOrder;
	extern int updateOrder;

	int getMasterTicket(wchar_t* comment);
	int getMasterTicket2(int magic);
	void writeMqlString(MqlString dest, wchar_t* source);
	void initZMQ();
	std::string WC2MB(const wchar_t* line);
	bool zmqReceiveOrders();
	void deInitZMQ();
	void LogFile(std::string fs);

	std::string sformat(const char *fmt, ...);

	extern std::map<std::string, SymbolInfo> SymbolInfos;

	#ifndef max
	#define max(a,b)            (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
	#define min(a,b)            (((a) < (b)) ? (a) : (b))
	#endif
	#define OP_BUY       0
	#define OP_SELL      1
	#define OP_BUYLIMIT  2
	#define OP_SELLLIMIT 3
	#define OP_BUYSTOP   4
	#define OP_SELLSTOP  5

}