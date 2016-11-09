#pragma once
#include "ffcTypes.h"

#define TIM_COOK        1
#define BILL_GATES      2
#define V_PUTIN			3
#define W_BUFFETT		4


namespace ffc {
	#pragma pack(push,1)
		extern int validOrder;
		extern int updateOrder;
	#pragma pack(pop,1)

	int getMasterTicket(wchar_t* comment);
	void writeMqlString(MqlString dest, wchar_t* source);
	void initZMQ();
	int zmqReceiveOrders(FfcOrder* master_orders);
	void deInitZMQ();
	int initCocktails(long acc_number);
	bool getCocktails(int provider, int name);
	//int* setCocktails(int provider[], int name);
	bool showCocktails(int name);
}