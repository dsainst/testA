#include "windows.h"
#include <ctime>
#include <iostream>
#include <thread>

#include "utils.h"
#include "ffcTypes.h"
#include "ActionManager.h"
#include "NetWorker.h"


#define POINT 0.0001
#define MIN_LOT 0.01
#pragma pack(push,1)
//---------- Секция общей памяти -----------------
//FfcOrder master_orders[MAX_ORDER_COUNT] = { 0 };
int			ordersCount = 0;
int			ordersTotal = 0;
bool		ordersValid = false;
bool		providerOk;
bool		transmitterBusy = false;
//int			cocktails[20][20] = { 0 };
//------------------------------------------------

//---------- Переменные ресивера -----------------

//---------- Инициализация -----------------
double balance = 0;

//---------- Основные параметры -----------------
int			masterTickets[MAX_ORDER_COUNT];
bool		recieverInit = false;
int			id_cocktails = 0;
//double		stoploss = 0;
double		procent = 0.02; // процент просирания
double		max_fail = 0; // максимальная сумма, которую не жалко просрать
int			mHistoryTickets[MAX_ORDER_COUNT]; // клиент-тикеты в истории

FfcOrder	client_orders[MAX_ORDER_COUNT] = { 0 };
int			ordersRCount = 0;
int			ordersCountHistory = 0;
bool		transmitterInit = false;

int			sign[2] = { 1,-1 };
double		mpo[2];
double		mpc[2];

double billingTimerUpdate = 0;

TerminalS	TermInfo[1] = { 0 };

nlohmann::json ffc::mainPackage;
#pragma pack(pop,1)

namespace ffc {

	void ffc_RDeInit() {
		deInitZMQ();
		recieverInit = false;
	}

	bool ffc_RInit(MqlAction* action_array, int length, double procentic, long login) {
		if (recieverInit) return false; //Повторная инициализация

		acc_number = login;
		ordersRCount = 0;
		recieverInit = true;

		/* связь с биллингом НАЧАЛО ------------------------->>>>>>>>>  */
		setAccount();
		updateAccountStep(&TermInfo[0]); 
		comSession();
		time_t timer;
		struct tm y2k = { 0 };

		y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
		y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

		time(&timer);  /* get current time; same as: timer = time(NULL)  */

		billingTimerUpdate = difftime(timer, mktime(&y2k));
		/* связь с биллингом КОНЕЦ ------------------------->>>>>>>>>  */

		initZMQ();

		initActions(action_array, length);

		id_cocktails = initCocktails(acc_number); //751137852

		if (procentic < 0.1)
			procent = procentic;

		std::wcout << "acc_number = " << acc_number << " procent_ = " << procentic << "\r\n";
		std::wcout << "Receiver inited.v3. Cocktails id = " << id_cocktails << "\r\n";
		return true; //Инициализация успешна
	}

	void ffc_RSetParam(double Rbalance, double Requity, double Rprofit, int Rmode, double Rfreemargin, int Rleverage, int Rlimit, int Rstoplevel, int Rstopmode, wchar_t* curr, wchar_t* comp, wchar_t* name, wchar_t* server) {
		if (AllocConsole()) {
			freopen("CONOUT$", "w", stdout);
			freopen("conout$", "w", stderr);
			SetConsoleOutputCP(CP_UTF8);// GetACP());
			SetConsoleCP(CP_UTF8);
		}
		//accountFreeMarginMode	accountLeverage	accountLimitOrders	accountName	accountNumber	accountServer	accountStopoutLevel	accountStopoutMode	accountTradeMode
		try
		{
			TermInfo[0] = { Rbalance, Requity, Rprofit, Rmode, Rfreemargin, Rleverage, Rlimit, Rstoplevel, Rstopmode, L"default", L"default", L"default", L"default" };
			wcscpy_s(TermInfo[0].currency, SYMBOL_LENGTH, curr);
			wcscpy_s(TermInfo[0].companyName, SYMBOL_MAX_LENGTH, comp);
			wcscpy_s(TermInfo[0].name, SYMBOL_MAX_LENGTH, name);
			wcscpy_s(TermInfo[0].server, SYMBOL_MAX_LENGTH, server);
		}
		catch (const std::exception&) { std::cout << "Dll is not inited!!!" << "\r\n"; }

		max_fail = procent * Rbalance;
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

	void ffc_RHistoryUpdate(int orderMagic, int orderTicket, __time64_t orderCloseTime, double orderClosePrice, int orderType, wchar_t* orderSymbol, double orderLots, double orderOpenPrice, __time64_t orderOpenTime, double orderTp, double orderSl) {
		mHistoryTickets[ordersCountHistory] = getMasterTicket2(orderMagic);
		ordersCountHistory++;
		updateOrderClosed(orderTicket, orderType, orderMagic, WC2MB(orderSymbol), orderLots, orderOpenTime, orderOpenPrice, orderTp, orderSl, orderCloseTime, orderClosePrice, 0);
	}

	void ffc_RClosedUpdate(int orderMagic, int orderTicket, __time64_t orderCloseTime, double orderClosePrice, int orderType, wchar_t* orderSymbol, double orderLots, double orderOpenPrice, __time64_t orderOpenTime, double orderTp, double orderSl) {
		updateOrderClosed(orderTicket, orderType, orderMagic, WC2MB(orderSymbol), orderLots, orderOpenTime, orderOpenPrice, orderTp, orderSl, orderCloseTime, orderClosePrice, 0);
	}

	int ffc_ROrdersUpdate(int ROrderTicket, int Rmagic, wchar_t* ROrderSymbol, int RorderType,
		double ROrderLots, double ROrderOpenPrice, __time64_t ROrderOpenTime, double ROrderTakeProfit, double ROrderStopLoss,
		__time64_t ROrderExpiration) {

		client_orders[ordersRCount] = { ROrderTicket, Rmagic, L"default", RorderType, ROrderLots, ROrderOpenPrice, ROrderOpenTime, ROrderTakeProfit, ROrderStopLoss, ROrderExpiration};

		wcscpy_s(client_orders[ordersRCount].symbol, SYMBOL_LENGTH, ROrderSymbol);
		//wcscpy_s(client_orders[ordersRCount].comment, COMMENT_LENGTH, ROrderComment);

		masterTickets[ordersRCount] = getMasterTicket2(Rmagic);

		//std::wcout << "Rmagic " << Rmagic << "\r\n";
		ordersRCount++;
		return ordersRCount;
	}

	void openOrdersUpdate(FfcOrder* client_orders) {
		auto key = 0;
		if (ordersRCount)
		while (key < ordersRCount) {
			auto order = &client_orders[key];
			addOpenOrder(order->ticket, order->magic, WC2MB(order->symbol), order->type, order->lots, order->openprice, order->opentime, order->tpprice, order->slprice, 0);
			key++;
			std::cout << "order->ticket=" << order->ticket << "\r\n";
		}
		//std::cout << "send to billing" << "\r\n";
		comSession();
	}


	int ffc_RGetJob() {
		resetActions();
		if (!threadActive)
			std::thread(zmqReceiveOrders).detach();
		mutex.lock();


		/* check billing connect on timer START */
		time_t timer;
		struct tm y2k = { 0 };
		double seconds;

		y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
		y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

		time(&timer);  /* get current time; same as: timer = time(NULL)  */

		seconds = difftime(timer, mktime(&y2k));

		//std::cout << "Current local time and date: " << seconds - billingTimerUpdate << "\r\n";

		if (abs(seconds - billingTimerUpdate) >= TIME_CONNECT_BILLING || newCom) {
			billingTimerUpdate = seconds;
			openOrdersUpdate(client_orders);
		}
		/* check billing connect on timer END */

		double deltaSL			= 0;
		double SL				= 0;
		double takep			= 0;
		double stoplevel		= 0;
		double slprice_min[2]	= { 0, 0 };
		double slprice_max[2]	= { 0, 0 };
		ordersTotal = msgServer.ordersCount;
		//std::wcout << "zmqReceiveOrders - " << msgServer.ordersCount << " ordersRCount - " << ordersRCount << "\r\n";
		for (int master_index = 0; master_index < ordersTotal; master_index++) {
			auto master_order = msgServer.orders + master_index;
			/*if (id_cocktails == 0 || getCocktails(master_order->magic, id_cocktails) == 0) {
				continue;
			}*/

			//для поиска нужного тикета
			providerOk = false;
			unsigned int vector_size = cocktails.size();
			for (int i = 0; i < vector_size; i++) {
				if (master_order->magic == cocktails[i] || master_order->magic == 1) {
					providerOk = true;
					break;
					//std::cout << cocktails[i] << " - " << master_order->magic << std::endl;
				}
			}
			if (vector_size == 0) {
				std::cout << "Check provider set!" << std::endl;
				providerOk = false;
			}

			if (!providerOk) continue;


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
							newCom = true;
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
					slprice_min[OP_BUY] = client_order->openprice - (3 * Info->points*sign[client_order->type]);
					slprice_max[OP_SELL] = client_order->openprice - (Info->points*sign[client_order->type]);
					slprice_min[OP_SELL] = client_order->openprice + (3 * Info->points*sign[client_order->type]);

					deltaSL = max_fail / (Info->tick_value * client_order->lots);
					SL = (client_order->type) ? client_order->openprice + (deltaSL * Info->points) : client_order->openprice - (deltaSL * Info->points);
					//std::wcout << "SymbolInfos deltaSL - " << deltaSL << " SymbolInfos SL - " << SL << "\r\n";

					if (client_order->type) { // sell // сдвиг в 0 надо сделать +3 -1
						if (master_order->slprice != 0 && ((master_order->slprice < SL && master_order->slprice > slprice_min[OP_SELL]) || (master_order->slprice < slprice_max[OP_SELL]))) {
							SL = master_order->slprice;
						}
						else {
							if (client_order->slprice >= slprice_max[OP_SELL]) {
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
							if (client_order->slprice <= slprice_max[OP_BUY]) {
								if (!(slprice_max[OP_BUY] && (mpc[client_order->type] - slprice_max[OP_BUY])*sign[client_order->type] <= stoplevel)) {
									SL = slprice_max[OP_BUY];
								}
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
				bool history_create = false;
				int ticket_temp = 0;
				// нужно сделать проверку в закрытых ордерах
				for (int history_index = 0; history_index < ordersCountHistory; history_index++) {
					if (mHistoryTickets[history_index] == master_order->ticket) {
						history_create = true;
						ticket_temp = mHistoryTickets[history_index];
						std::wcout << "ticket is master_order->ticket - " << master_order->ticket << "\r\n";
					}
					//std::cout << "ticket_temp = " << mHistoryTickets[history_index] << " ticket_master - " << master_order->ticket << " magic_master - " << master_order->magic << "\r\n";
					//std::wcout << "history_index - " << history_index  << " mHistoryTickets - " << mHistoryTickets[history_index] << " master_order->ticket - " << master_order->ticket << "\r\n";
				}
				if (!history_create) {
					std::wcout << "master_order->ticket - " << master_order->ticket << "\r\n";
					mHistoryTickets[ordersCountHistory] = master_order->ticket;
					ordersCountHistory++;
					master_order->lots = balance / master_order->lots;
					createOrder(master_order);
					newCom = true;
				} else std::wcout << "this ticket is closed - " << ticket_temp << "\r\n";
			}
			master_order = 0;
		}
		for (int client_index = 0; client_index < ordersRCount; client_index++) {
			auto client_order = client_orders + client_index;

			if (client_order->expiration != 1 && msgServer.validation) {
				mHistoryTickets[ordersCountHistory] = getMasterTicket2(client_order->magic);
				ordersCountHistory++;
				if (client_order->type < 2) {
					closeOrder(client_order);
					newCom = true;
				}
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