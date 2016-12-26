#pragma once
#include <string>
#include "ffcTypes.h"

#pragma pack(push,1)
#define JOB_EXIT        0
#define JOB_CREATE      1
#define JOB_MODIFY      2
#define JOB_DELETE      3
#define JOB_CLOSE       4
#define JOB_PRINT_ORDER 5
#define JOB_PRINT_TEXT  6
#define JOB_DRAW_ORDER  7
#define JOB_SHOW_VALUE  8
#define JOB_MSG_BOX     9
#pragma pack(pop,1)

namespace ffc {
	//Структура для передачи действий
#pragma pack(push,1)
	struct MqlAction			// 80 bytes
	{
		int         actionId;	// 4 bytes
		int         ticket;		// 4 bytes
		int         type;		// 4 bytes
		int         magic;		// 4 bytes
		MqlString	symbol;		// 12 bytes
		double      lots;		// 8 bytes
		double      openprice;	// 8 bytes
		double      tpprice;	// 8 bytes
		double      slprice;	// 8 bytes
		__time64_t	expiration;	// 8 bytes
//		MqlString	comment;	// 12 bytes
	};
#pragma pack(pop,1)

	extern int actionsCount;
	extern int actionsMaxCount;

	extern double deltaStopLevel;
	extern double deltaFreezeLevel;
	extern MqlAction* actions;

	void initActions(MqlAction* arrayPtr, int length);

	void resetActions();

	void createOrder(wchar_t* symbol, int type, double lots, double openPrice, double slPrice, double tpPrice, wchar_t* comment = L"", int cfgMagic = 0);
	void createOrder(FfcOrder* order);
	void modOrder(int ticket, int type, double lots, double openprice, double slprice, double tpprice, wchar_t* symbol);
	void deleteOrder(int ticket);
	void closeOrder(int ticket, double lots, double openprice);
	void closeOrder(FfcOrder* order);
	void deleteOrder(FfcOrder* order);

	void showValue(int line, wchar_t* value);

	double normLotMin(double value, wchar_t* symbol_name);
	double normPrice(double value, wchar_t* symbol_name);
	double normLot(double value, wchar_t* symbol_name);
	void terminalInfoCalc(wchar_t* symbol_name);
//	bool check_sl(int type, double low_price, double high_price);
//	int check_new(int type, double lots, double openprice, double slprice, double tpprice);

}