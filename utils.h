#pragma once
#include <mutex>
#include "ffcTypes.h"

#define TIM_COOK        1
#define BILL_GATES      2
#define V_PUTIN			3
#define W_BUFFETT		4


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
	void zmqReceiveOrders();
	void deInitZMQ();
	int initCocktails(long acc_number);
	bool getCocktails(int provider, int name);
	//int* setCocktails(int provider[], int name);
	bool showCocktails(int name);
}