#include "windows.h"
#include <ctime>
#include <iostream>
#include <thread>
#include "utils.h"
#include "ffcTypes.h"
#include "ActionManager.h"

#define MAX_ORDER_COUNT	200
#define POINT 0.0001

//---------- Секция общей памяти -----------------
#pragma data_seg ("shared")
FfcOrder master_orders[MAX_ORDER_COUNT] = { 0 };
int ordersCount = 0;
int ordersTotal = 0;
bool ordersValid = false;
bool transmitterBusy = false;
int cocktails[20][20] = { 0 };
#pragma data_seg()
#pragma comment(linker, "/SECTION:shared,RWS")
//------------------------------------------------

//---------- Переменные ресивера -----------------
int masterTickets[MAX_ORDER_COUNT];
bool recieverInit = false;
long id_login = 0;
int id_cocktails = 0;

//---------- Переменные трансмиттера -------------
FfcOrder client_orders[MAX_ORDER_COUNT] = { 0 };
int ordersRCount = 0;
bool transmitterInit = false;


namespace ffc {

	void ffc_RDeInit() {
		deInitZMQ();
		recieverInit = false;
	}

	bool ffc_RInit(MqlAction* action_array, int length, long login) {
		if (recieverInit) return false; //Повторная инициализация
		if (AllocConsole()) {
			freopen("CONOUT$", "w", stdout);
			freopen("conout$", "w", stderr);
			SetConsoleOutputCP(CP_UTF8);// GetACP());
			SetConsoleCP(CP_UTF8);
		}

		initZMQ();

		id_login = login;
		ordersRCount = 0;
		recieverInit = true;
		initActions(action_array, length);

		id_cocktails = initCocktails(id_login); //751137852

		std::wcout << "Receiver inited. Cocktails id = " << id_cocktails << "\r\n";
		return true; //Инициализация успешна
	}

	int ffc_ROrdersUpdate(int ROrderTicket, int Rmagic, wchar_t* ROrderSymbol, int RorderType,
		double ROrderLots, double ROrderOpenPrice, __time64_t ROrderOpenTime,
		double ROrderTakeProfit, double ROrderStopLoss, double  ROrderClosePrice, __time64_t  ROrderCloseTime,
		__time64_t ROrderExpiration, double  ROrderProfit, double  ROrderCommission, double  ROrderSwap, wchar_t* ROrderComment) {

		client_orders[ordersRCount] = { ROrderTicket, Rmagic, L"default", RorderType, ROrderLots, ROrderOpenPrice, ROrderOpenTime, ROrderTakeProfit, ROrderStopLoss, ROrderClosePrice, ROrderCloseTime, ROrderExpiration, ROrderProfit, ROrderCommission, ROrderSwap, L"" };

		wcscpy_s(client_orders[ordersRCount].symbol, SYMBOL_LENGTH, ROrderSymbol);
		wcscpy_s(client_orders[ordersRCount].comment, COMMENT_LENGTH, ROrderComment);

		masterTickets[ordersRCount] = getMasterTicket(ROrderComment);

		std::wcout << "updateOrder " << updateOrder << "\r\n";
		ordersRCount++;
		return ordersRCount;
	}


	int ffc_RGetJob() {
		while (transmitterBusy) {  //ждем когда трансмиттер закончит свою работу
			std::this_thread::sleep_for(std::chrono::milliseconds(25));  //что бы не перегружать систему
		}
		ffc::resetActions();
		ordersTotal = zmqReceiveOrders(master_orders);
		std::wcout << "zmqReceiveOrders - " << master_orders[0].ticket << " totalCount - " << ordersTotal << "\r\n";
		for (int master_index = 0; master_index < ordersTotal; master_index++) {
			auto master_order = master_orders + master_index;

			if (id_cocktails == 0 || getCocktails(master_order->magic, id_cocktails) == 0) {
				continue;
			}

			int found = false;
			for (int client_index = 0; client_index < ordersRCount; client_index++) {
				if (master_order->ticket == masterTickets[client_index]) {
					found = true;
					auto client_order = client_orders + client_index;
					client_order->closetime = 1;
					if (abs(master_order->slprice - client_order->slprice) > POINT ||
						abs(master_order->tpprice - client_order->tpprice) > POINT) {
						modOrder(client_order->ticket, client_order->openprice, master_order->slprice, master_order->tpprice, client_order->symbol);
					}
					break;
				}
			}
			if (!found) {
				createOrder(master_order);
			}
		}
		for (int client_index = 0; client_index < ordersRCount; client_index++) {
			auto client_order = client_orders + client_index;

			if (client_order->closetime == 0 && validOrder) {
				if (client_order->type < 2)
					closeOrder(client_order);
				else
					deleteOrder(client_order);
			}
		}
		ordersRCount = 0;
		return actionsCount;
	}

	//--------------------------------------------------------------------------------------------
}