#include "windows.h"
#include <ctime>
#include <iostream>
#include <thread>
#include <thread>

#include "utils.h"
#include "ffcTypes.h"
#include "ActionManager.h"


#define POINT 0.0001
#define MIN_LOT 0.01

//---------- Секция общей памяти -----------------
#pragma data_seg ("shared")
//FfcOrder master_orders[MAX_ORDER_COUNT] = { 0 };
int ordersCount = 0;
int ordersTotal = 0;
bool ordersValid = false;
bool transmitterBusy = false;
int cocktails[20][20] = { 0 };
std::thread receiveThread;
#pragma data_seg()
#pragma comment(linker, "/SECTION:shared,RWS")
//------------------------------------------------

//---------- Переменные ресивера -----------------
int masterTickets[MAX_ORDER_COUNT];
double balance = 0;
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

		std::wcout << "Receiver inited.v3. Cocktails id = " << id_cocktails << "\r\n";
		return true; //Инициализация успешна
	}

	void ffc_RSetBalance(double Rbalance) {
		balance = Rbalance;
	}

	int ffc_ROrdersUpdate(int ROrderTicket, int Rmagic, wchar_t* ROrderSymbol, int RorderType,
		double ROrderLots, double ROrderOpenPrice, double ROrderTakeProfit, double ROrderStopLoss, 
		__time64_t ROrderExpiration) {
		client_orders[ordersRCount] = { ROrderTicket, Rmagic, L"default", RorderType, ROrderLots, ROrderOpenPrice, ROrderTakeProfit, ROrderStopLoss, ROrderExpiration};

		wcscpy_s(client_orders[ordersRCount].symbol, SYMBOL_LENGTH, ROrderSymbol);
		//wcscpy_s(client_orders[ordersRCount].comment, COMMENT_LENGTH, ROrderComment);

		masterTickets[ordersRCount] = getMasterTicket2(Rmagic);

		//std::wcout << "Rmagic " << Rmagic << "\r\n";
		ordersRCount++;
		return ordersRCount;
	}


	int ffc_RGetJob() {
		ffc::resetActions();
		if (!threadActive)
			std::thread(zmqReceiveOrders).detach();
		mutex.lock();
		ordersTotal = msg.ordersCount;
		//std::wcout << "zmqReceiveOrders - " << msg.ordersCount << " ordersRCount - " << ordersRCount << "\r\n";
		for (int master_index = 0; master_index < ordersTotal; master_index++) {
			auto master_order = msg.orders + master_index;
			/*if (id_cocktails == 0 || getCocktails(master_order->magic, id_cocktails) == 0) {
				continue;
			}*/
			if (master_order->magic != 1593142 && master_order->magic != 1503204 && master_order->magic != 1346753 && master_order->magic != 1)
				continue;

			//std::wcout << "master_order ticket - " << master_order->ticket << "\r\n";
			int found = false;
			for (int client_index = 0; client_index < ordersRCount; client_index++) {
				//std::wcout << "master_order ticket - " << masterTickets[client_index] << "\r\n";
				if (master_order->ticket == masterTickets[client_index]) {
					found = true;
					auto client_order = client_orders + client_index;
					client_order->expiration = 1; // для защиты от удаления
					if (master_order->magic == 1) { // проверка частичного закрытия ордера
						double diff = client_order->lots - (balance / master_order->lots);
						//std::wcout << "diff - " << diff << " client_order->lots - " << client_order->lots << "\r\n";
						if (diff>=MIN_LOT) {
							client_order->lots = diff;
							closeOrder(client_order); 
						}
					}
					
					if (abs(master_order->slprice - client_order->slprice) > POINT ||
						abs(master_order->tpprice - client_order->tpprice) > POINT) {
						modOrder(client_order->ticket, client_order->type, client_order->openprice, master_order->slprice, master_order->tpprice, client_order->symbol);
					}
					break;
				}
			}
			if (!found) {
				master_order->lots = balance / master_order->lots;
				createOrder(master_order);
			}
		}
		for (int client_index = 0; client_index < ordersRCount; client_index++) {
			auto client_order = client_orders + client_index;

			if (client_order->expiration != 1 && msg.validation) {
				if (client_order->type < 2)
					closeOrder(client_order);
				else
					deleteOrder(client_order);
			}
		}
		std::wcout << "Receiver actionsCount - " << actionsCount << ". \r\n";
		ordersRCount = 0;
		mutex.unlock();
		return actionsCount;
	}

	//--------------------------------------------------------------------------------------------
}