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
//FfcOrder master_orders[MAX_ORDER_COUNT] = { 0 };
int ordersCount = 0;
int ordersTotal = 0;
bool ordersValid = false;
bool transmitterBusy = false;
int cocktails[20][20] = { 0 };
std::thread receiveThread;
//------------------------------------------------

//---------- Переменные ресивера -----------------
int masterTickets[MAX_ORDER_COUNT];
double balance = 0;
bool recieverInit = false;
long id_login = 0;
int id_cocktails = 0;
double stoploss = 0;
double procent = 0.02; // процент просирания
double max_fail = 0; // максимальная сумма, которую не жалко просрать
int mHistoryTickets[MAX_ORDER_COUNT]; // клиент тикеты в истории

FfcOrder client_orders[MAX_ORDER_COUNT] = { 0 };
int ordersRCount = 0;
int ordersCountHistory = 0;
bool transmitterInit = false;

int    sign[2] = { 1,-1 };
double mpo[2];
double mpc[2];


namespace ffc {

	void ffc_RDeInit() {
		deInitZMQ();
		recieverInit = false;
	}

	bool ffc_RInit(MqlAction* action_array, int length, double procentic, long login) {
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

		if (procentic < 0.1)
			procent = procentic;

		std::wcout << "id_login = " << id_login << " procent_ = " << procentic << "\r\n";


		std::wcout << "Receiver inited.v3. Cocktails id = " << id_cocktails << "\r\n";
		return true; //Инициализация успешна
	}

	void ffc_RInitSymbols(wchar_t* name, double min_lot, double max_lot, double lotstep, double tick_value, double points, double lotsize, double ask, double bid, double digits, double stoplevel, double freezelevel, double trade_allowed) {
		auto Info = &SymbolInfos[WC2MB(name)];
		Info->min_lot = min_lot;
		Info->max_lot = max_lot;
		Info->lotstep = lotstep;
		Info->tick_value = tick_value;
		Info->points = points;
		Info->lotsize = lotsize;
		Info->ask = ask;
		Info->bid = bid;
		Info->digits = digits;
		Info->stoplevel = stoplevel;
		Info->freezelevel = freezelevel;
		Info->trade_allowed = trade_allowed;

	}

	void ffc_RSetBalance(double Rbalance) {
		balance = Rbalance;
		max_fail = procent * balance;
	}

	void ffc_RHistoryUpdate(int orderMagic) {
		mHistoryTickets[ordersCountHistory] = getMasterTicket2(orderMagic);
		ordersCountHistory++;
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
		double deltaSL			= 0;
		double SL				= 0;
		double takep			= 0;
		double stoplevel		= 0;
		double slprice_min[2]	= { 0, 0 };
		double slprice_max[2]	= { 0, 0 };
		ordersTotal = msg.ordersCount;
		//std::wcout << "zmqReceiveOrders - " << msg.ordersCount << " ordersRCount - " << ordersRCount << "\r\n";
		for (int master_index = 0; master_index < ordersTotal; master_index++) {
			auto master_order = msg.orders + master_index;
			/*if (id_cocktails == 0 || getCocktails(master_order->magic, id_cocktails) == 0) {
				continue;
			}*/
			if (master_order->magic != 1593142 && master_order->magic != 1503204 && master_order->magic != 1346753 && master_order->magic != 1)
				continue;

			int found = false;
			for (int client_index = 0; client_index < ordersRCount; client_index++) {
				if (master_order->ticket == masterTickets[client_index]) {
					found = true;
					auto client_order = client_orders + client_index;
					client_order->expiration = 1; // для защиты от удаления
					if (master_order->magic == 1) { // проверка частичного закрытия ордера
						double diff = client_order->lots - (balance / master_order->lots);
						if (diff>=MIN_LOT) {
							client_order->lots = diff;
							closeOrder(client_order);
							mHistoryTickets[ordersCountHistory] = getMasterTicket2(client_order->magic);
							ordersCountHistory++;
						}
					}
					auto Info = &SymbolInfos[WC2MB(client_order->symbol)]; 

					mpc[OP_BUY] = Info->bid;
					mpc[OP_SELL] = Info->ask;
					//std::wcout << "SymbolInfos tick_value - " << Info->tick_value << " SymbolInfos POINT - " << Info->points << " SymbolInfos lotsize - " << Info->lotsize << "\r\n";

					// рассчитаем безубыточные стоплосы
					stoplevel = Info->stoplevel * Info->points;
					slprice_max[OP_BUY] = client_order->openprice + (Info->points*sign[client_order->type]);
					slprice_min[OP_BUY] = client_order->openprice - (3*Info->points*sign[client_order->type]);
					slprice_max[OP_SELL] = client_order->openprice - (Info->points*sign[client_order->type]);
					slprice_min[OP_SELL] = client_order->openprice + (3 * Info->points*sign[client_order->type]);

					deltaSL = max_fail / (Info->tick_value * client_order->lots);
					SL = (client_order->type) ? client_order->openprice + (deltaSL * Info->points) : client_order->openprice - (deltaSL * Info->points);
					std::wcout << "SymbolInfos deltaSL - " << deltaSL << " SymbolInfos SL - " << SL << "\r\n";

					if (client_order->type) { // sell // сдвиг в 0 надо сделать +3 -1
						if (master_order->slprice != 0 && ((master_order->slprice < SL && master_order->slprice > slprice_min[OP_SELL]) || (master_order->slprice < slprice_max[OP_SELL]))) {
							SL = master_order->slprice;
						}
						else {
							if (client_order->slprice >= slprice_min[OP_SELL]) {
								if (!(slprice_max[OP_SELL] && (mpc[client_order->type] - slprice_max[OP_SELL])*sign[client_order->type] <= stoplevel))
									SL = slprice_max[OP_SELL];
							} // иначе стоит максимум безубытка, поэтому ничего не трогаем
						}
					}
					else { // buy  //сдвиг -3 +1
						if (master_order->slprice != 0 && ((master_order->slprice > SL && master_order->slprice < slprice_min[OP_BUY]) || (master_order->slprice > slprice_max[OP_BUY]))) {
							SL = master_order->slprice;
						}
						else {
							if (client_order->slprice <= slprice_min[OP_BUY]) {
								if (!(slprice_max[OP_BUY] && (mpc[client_order->type] - slprice_max[OP_BUY])*sign[client_order->type] <= stoplevel))
									SL = slprice_max[OP_BUY];
							} // иначе стоит максимум безубытка, поэтому ничего не трогаем
						}
					}

					//std::wcout << "SymbolInfos deltaSL - " << deltaSL << " SymbolInfos SL - " << SL << "\r\n";

					takep = client_order->tpprice;
					if (abs(master_order->tpprice - client_order->tpprice) > POINT) takep = master_order->tpprice;

					if (abs(SL - client_order->slprice) > POINT || abs(takep - client_order->tpprice) > POINT)
						if (!(client_order->slprice && (mpc[client_order->type] - client_order->slprice)*sign[client_order->type] <= stoplevel))
							modOrder(client_order->ticket, client_order->type, client_order->lots, client_order->openprice, SL, takep, client_order->symbol);

					break;
				}
			}
			if (!found) {
				bool history_create = true;
				int ticket_temp = 0;
				// нужно сделать проверку в закрытых ордерах
				for (int history_index = 0; history_index < ordersCountHistory; history_index++) {
					if (mHistoryTickets[history_index] == master_order->ticket) {
						history_create = false;
						ticket_temp = mHistoryTickets[history_index];
					}
					//std::wcout << "history_index - " << history_index  << " mHistoryTickets - " << mHistoryTickets[history_index] << " master_order->ticket - " << master_order->ticket << "\r\n";
				}
				if (history_create) {
					//std::wcout << "master_order->ticket - " << master_order->ticket << "\r\n";
					mHistoryTickets[ordersCountHistory] = master_order->ticket;
					ordersCountHistory++;
					master_order->lots = balance / master_order->lots;
					createOrder(master_order);
				} //else std::wcout << "this ticket is closed - " << ticket_temp << "\r\n";
			}
			master_order = 0;
		}
		for (int client_index = 0; client_index < ordersRCount; client_index++) {
			auto client_order = client_orders + client_index;

			if (client_order->expiration != 1 && msg.validation) {
				mHistoryTickets[ordersCountHistory] = getMasterTicket2(client_order->magic);
				ordersCountHistory++;
				if (client_order->type < 2)
					closeOrder(client_order);
				else
					deleteOrder(client_order);
			}
		}
		//std::wcout << "Receiver actionsCount - " << actionsCount << ". \r\n";
		ordersRCount = 0;
		mutex.unlock();
		return actionsCount;
	}

	//--------------------------------------------------------------------------------------------
}