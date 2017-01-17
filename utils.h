#pragma once
#include <mutex>
#include <map>
#include "ffcTypes.h"

#pragma pack(push,1)
#define TIM_COOK        1
#define BILL_GATES      2
#define V_PUTIN			3
#define W_BUFFETT		4
#pragma pack(pop,1)

namespace ffc {

	extern FfcMsg msg;
	extern bool threadActive;
	extern std::mutex mutex;

	extern int validOrder;
	extern int updateOrder;

	int getMasterTicket(wchar_t* comment);
	int getMasterTicket2(int magic);
	void writeMqlString(MqlString dest, wchar_t* source);
	void initZMQ();
	std::string WC2MB(const wchar_t* line);
	void zmqReceiveOrders();
	void deInitZMQ();
	int initCocktails(long acc_number);
	bool getCocktails(int provider, int name);
	//int* setCocktails(int provider[], int name);
	bool showCocktails(int name);

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