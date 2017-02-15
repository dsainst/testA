#include "windows.h"
#include <ctime>
#include <iostream>
#include <thread>

#include "utils.h"
#include "ffcTypes.h"
#include "ActionManager.h"
#include "NetWorker.h"

#define OPEN_ORDERS_LIMIT 250
//#define POINT 0.0001
#define MIN_LOT 0.01
#pragma pack(push,1)
//---------- —екци€ общей пам€ти -----------------
//FfcOrder master_orders[MAX_ORDER_COUNT] = { 0 };
int			ordersCount = 0;
int			ordersTotal = 0;
bool		ordersValid = false;
bool		providerOk;
bool		transmitterBusy = false;
//int			cocktails[20][20] = { 0 };
//------------------------------------------------

//---------- ѕеременные ресивера -----------------

//---------- »нициализаци€ -----------------
double balance = 0;

//---------- ќсновные параметры -----------------
int			masterTickets[MAX_ORDER_COUNT];
bool		recieverInit = false;
//double		stoploss = 0;
double		procent = 0.02; // процент просирани€
double		max_fail = 0; // максимальна€ сумма, которую не жалко просрать
int			mHistoryTickets[MAX_ORDER_HISTORY_COUNT]; // клиент-тикеты в истории + открытые ордера

FfcOrder	client_orders[MAX_ORDER_COUNT] = { 0 };
int			ordersRCount = 0;
int			ordersCountHistory = 0;
int			ordersCountHistoryMax = 0;
bool		transmitterInit = false;

int			sign[2] = { 1,-1 };
double		mpo[2];
double		mpc[2];

int depolot = 0;

double billingTimerUpdate = 0;

TerminalS	TermInfo[1] = { 0 };

bool history_update;

nlohmann::json ffc::mainPackage;
#pragma pack(pop,1)

namespace ffc {

	bool ffc_RInit(MqlAction* action_array, int length, double procentic, int lotsize, long login, wchar_t* path) {
		if (recieverInit) return false; //ѕовторна€ инициализаци€

		acc_number = login;
		ordersRCount = 0;
		recieverInit = true;
		history_update = true;
		depolot = lotsize;

		/* св€зь с биллингом Ќј„јЋќ ------------------------->>>>>>>>>  */
		setAccount();
		updateAccountStep(&TermInfo[0]); 
		comSession();

		ordersCountHistory = 0;
		ordersCountHistoryMax = 0;

		// запускаем таймер дл€ св€зи с биллингом
		time_t timer;
		struct tm y2k = { 0 };

		y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
		y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

		time(&timer);  /* get current time; same as: timer = time(NULL)  */

		billingTimerUpdate = difftime(timer, mktime(&y2k));
		/* св€зь с биллингом  ќЌ≈÷ ------------------------->>>>>>>>>  */

		initZMQ();

		initActions(action_array, length);

		if (procentic < 0.1)
			procent = procentic;

		std::wcout << "acc_number = " << acc_number << " procent_ = " << procentic << "\r\n";

		std::ostringstream oss;
		oss << "acc_number = ";
		oss << acc_number;
		LogFile(oss.str());
		std::wcout << "Receiver inited.v3.2 \r\n";
		return true; //»нициализаци€ успешна
	}

	bool ffc_Rworkout() {
		if (workStop) {
			// надо запустить таймер, чтобы провер€л соединение с сервером (раз в минуту)
			/* check billing connect on timer START */
			time_t timer;
			struct tm y2k = { 0 };
			double seconds;

			y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
			y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

			time(&timer);  /* get current time; same as: timer = time(NULL)  */

			seconds = difftime(timer, mktime(&y2k));

			//std::cout << "Current local time and date: " << seconds - billingTimerUpdate << "\r\n";

			if (abs(seconds - billingTimerUpdate) >= TIME_CONNECT_BILLING_UNAUTHORIZED) {
				billingTimerUpdate = seconds;
				setAccount();
				updateAccountStep(&TermInfo[0]);
				comSession();
			}
			/* check billing connect on timer END */
		}
		return workStop;
	}

	void ffc_RSetParam(double Rbalance, double Requity, double Rprofit, int Rmode, double Rfreemargin, int Rleverage, int Rlimit, int Rstoplevel, int Rstopmode, wchar_t* curr, wchar_t* comp, wchar_t* name, wchar_t* server) {
		if (AllocConsole()) {
			freopen("CONOUT$", "w", stdout);
			freopen("conout$", "w", stderr);
			SetConsoleOutputCP(CP_UTF8);// GetACP());
			SetConsoleCP(CP_UTF8);
		}
		try
		{
			TermInfo[0] = { Rbalance, Requity, Rprofit, Rmode, Rfreemargin, Rleverage, Rlimit, Rstoplevel, Rstopmode, L"default", L"default", L"default", L"default" };
			wcscpy_s(TermInfo[0].currency, SYMBOL_LENGTH, curr);
			wcscpy_s(TermInfo[0].companyName, SYMBOL_MAX_LENGTH, comp);
			wcscpy_s(TermInfo[0].name, SYMBOL_MAX_LENGTH, name);
			wcscpy_s(TermInfo[0].server, SYMBOL_MAX_LENGTH, server);
		}
		catch (const std::exception&) { std::cout << "Dll is not inited!!!" << "\r\n"; }
		balance = Rbalance;
		max_fail = procent * Rbalance;
	}

	void ffc_RInitSymbols(wchar_t* name, double min_lot, double max_lot, double lotstep, double tick_value, double points, double lotsize, double digits, double stoplevel, double freezelevel, double trade_allowed) {
		auto Info = &SymbolInfos[WC2MB(name)];
		Info->min_lot = min_lot;
		Info->max_lot = max_lot;
		Info->lotstep = lotstep;
		Info->tick_value = tick_value;
		Info->points = points;
		Info->lotsize = lotsize;
		Info->digits = digits;
		Info->stoplevel = stoplevel;
		Info->freezelevel = freezelevel;
		Info->trade_allowed = trade_allowed;
	}

	void ffc_RHistoryUpdate(int orderMagic, int orderTicket, __time64_t orderCloseTime, double orderClosePrice, int orderType, wchar_t* orderSymbol, double orderLots, double orderOpenPrice, __time64_t orderOpenTime, double orderTp, double orderSl, double orderSwap, double orderCom, double orderProf) {
		mHistoryTickets[ordersCountHistory] = getMasterTicket2(orderMagic);
		ordersCountHistory++; 
		if (ordersCountHistory > 499) {
			ordersCountHistoryMax = 500;
			ordersCountHistory = 0;
		} else {
			if (ordersCountHistoryMax != 500)
				ordersCountHistoryMax = ordersCountHistory;
		}

		//std::cout << "Order History ticket = " << orderTicket << " mapedTicket = " << getMasterTicket2(orderMagic) << " index = " << ordersCountHistoryMax << "\r\n";

		updateOrderClosed(orderTicket, orderType, orderMagic, WC2MB(orderSymbol), orderLots, orderOpenTime, orderOpenPrice, orderTp, orderSl, orderCloseTime, orderClosePrice, orderSwap + orderCom + orderProf);
	}

	void ffc_RClosedUpdate(int orderMagic, int orderTicket, __time64_t orderCloseTime, double orderClosePrice, int orderType, wchar_t* orderSymbol, double orderLots, double orderOpenPrice, __time64_t orderOpenTime, double orderTp, double orderSl, double orderSwap, double orderCom, double orderProf) {
		updateOrderClosed(orderTicket, orderType, orderMagic, WC2MB(orderSymbol), orderLots, orderOpenTime, orderOpenPrice, orderTp, orderSl, orderCloseTime, orderClosePrice, orderSwap + orderCom + orderProf);
	}

	int ffc_RInterestTickets() {
		int ticket_;
		auto itr = interestClosedTickets.begin();
		if (itr != interestClosedTickets.end()) {
			std::cout << "interestClosedTickets = " << *itr <<"\r\n";
			ticket_ = *itr; 
			interestClosedTickets.erase(itr);
		} else return 0;
		return ticket_;
		//interestTickets.clear();
	}

	void openOrdersUpdate(FfcOrder* client_orders) { // ќткрытые ордера отправл€ем на биллинг
		auto key = 0;
		if (ordersRCount)
			while (key < ordersRCount) {
				auto order = &client_orders[key];
				if (order->type == 0 || order->type == 1){
					addOpenOrder(order->ticket, order->magic, WC2MB(order->symbol), order->type, order->lots, order->openprice, order->opentime, order->tpprice, order->slprice, 0);
					std::cout << "send order->ticket=" << order->ticket << "\r\n";
				}
				key++;
			}
		//std::cout << "send to billing" << "\r\n";
		comSession();
	}

	int ffc_ROrdersUpdate(int ROrderTicket, int Rmagic, wchar_t* ROrderSymbol, int RorderType,
		double ROrderLots, double ROrderOpenPrice, __time64_t ROrderOpenTime, double ROrderTakeProfit, double ROrderStopLoss,
		__time64_t ROrderExpiration, double tickvalue, double  point, double ask, double bid) {

		client_orders[ordersRCount] = { ROrderTicket, 0, Rmagic, L"default", RorderType, ROrderLots, ROrderOpenPrice, ROrderOpenTime, ROrderTakeProfit, ROrderStopLoss, 0, 0, ROrderExpiration};


		wcscpy_s(client_orders[ordersRCount].symbol, SYMBOL_LENGTH, ROrderSymbol);
		//wcscpy_s(client_orders[ordersRCount].comment, COMMENT_LENGTH, ROrderComment);

		auto Info = &SymbolInfos[WC2MB(client_orders[ordersRCount].symbol)];
		Info->ask = ask;
		Info->bid = bid;

		std::lock_guard<std::mutex> locker(mutex);
		int _ticket = 0;

		_ticket = getMasterTicket2(Rmagic);

		masterTickets[ordersRCount] = _ticket;

		bool history_flag_add = true;

		/*if (history_update) { // если давали команду на открытие нового ордера, то провер€ем историю и записываем новые ордера
			for (int history_index = 0; history_index < ordersCountHistory; history_index++) {
				if (mHistoryTickets[history_index] == _ticket) {
					history_flag_add = false;
					break;
				}
			}
		}
		if (history_flag_add && history_update) { // добавл€ем в историю открытые ордера, чтобы при закрытии их вручную или по sl/tp они не открывались заново
			std::cout << "add ticket in history = " << _ticket << " magic number = " << Rmagic << " count = " << ordersCountHistory << "\r\n";
			mHistoryTickets[ordersCountHistory] = _ticket;
			ordersCountHistory++;
			newCom = true; // двойна€ отправка будет при инициализации, отправл€ет на биллинг новые, только что открытые ордера
		}*/

		ordersRCount++;
		return ordersRCount;
	}


	int ffc_RGetJob(bool flag_no_open) {
		resetActions();
		if (history_update) history_update = false;
		if (!threadActive)
			std::thread(zmqReceiveOrders).detach();
		std::lock_guard<std::mutex> locker(mutex);
		// выходим сразу, если нет провайдеров
		if (cocktails.size() == 0) {
			providerOk = false;
			return 0;
		}

		/* check billing connect on timer START */
		time_t timer;
		struct tm y2k = { 0 };
		double seconds;

		y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
		y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;
		time(&timer);
		seconds = difftime(timer, mktime(&y2k));

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
		double digits[6] = { 0, 0.1, 0.01, 0.001, 0.0001, 0.00001 };
		ordersTotal = msgServer.ordersCount;
		//std::wcout << "zmqReceiveOrders - " << msgServer.ordersCount << " ordersRCount - " << ordersRCount << " ordersTotal - " << ordersTotal << "\r\n";

		for (int master_index = 0; master_index < ordersTotal; master_index++) {
			auto master_order = msgServer.orders + master_index;

			//std::cout << "Order master ticket = " << master_order->ticket << " mapped ticket = " << master_order->mapedTicket << "\r\n";

			//дл€ поиска нужного тикета
			providerOk = false;
			unsigned int vector_size = cocktails.size();
			for (int i = 0; i < vector_size; i++) {
				if (master_order->magic == cocktails[i] || master_order->magic == 1) {
					providerOk = true;
					break;
					//std::cout << cocktails[i] << " - " << master_order->magic << " - " << i << std::endl;
				}
			}
			if (vector_size == 0) {
				std::cout << "Check provider set!" << std::endl;
				providerOk = false;
			}

			if (!providerOk) continue;

			int found = false;
			for (int client_index = 0; client_index < ordersRCount; client_index++) {
				if (master_order->mapedTicket != masterTickets[client_index]) continue;
				found = true;
				auto client_order = client_orders + client_index;
				client_order->expiration = 1; // дл€ защиты от удалени€
				if (master_order->magic == 1) { // проверка частичного закрыти€ ордера
					double diff = 0;
					if (depolot) {
						auto lotsize = msgServer.balance / master_order->lots;
						diff = client_order->lots - (balance / lotsize);
					}
					else {
						diff = client_order->lots - master_order->lots;
					}
					if (diff>=MIN_LOT) {
						client_order->lots = diff;
						closeOrder(client_order);
						newCom = true;
						mHistoryTickets[ordersCountHistory] = getMasterTicket2(client_order->magic);
						ordersCountHistory++;
						if (ordersCountHistory > 499) {
							ordersCountHistoryMax = 500;
							ordersCountHistory = 0;
						}
						else {
							if (ordersCountHistoryMax != 500)
								ordersCountHistoryMax = ordersCountHistory;
						}
					}
				}
				auto Info = &SymbolInfos[WC2MB(client_order->symbol)]; 

				mpc[OP_BUY] = Info->bid;
				mpc[OP_SELL] = Info->ask;
				//std::wcout << "SymbolInfos tick_value - " << Info->tick_value << " SymbolInfos POINT - " << Info->points << " SymbolInfos lotsize - " << Info->lotsize << "\r\n";

				// рассчитаем безубыточные стоплосы
				/*stoplevel = Info->stoplevel * Info->points;
				slprice_max[OP_BUY] = client_order->openprice + (3 * Info->points*sign[client_order->type]);
				slprice_min[OP_BUY] = client_order->openprice - (3 * Info->points*sign[client_order->type]);
				slprice_max[OP_SELL] = client_order->openprice - (3 * Info->points*sign[client_order->type]);
				slprice_min[OP_SELL] = client_order->openprice + (3 * Info->points*sign[client_order->type]);


				deltaSL = max_fail / (Info->tick_value * client_order->lots);
				SL = (client_order->type) ? client_order->openprice + (deltaSL * Info->points) : client_order->openprice - (deltaSL * Info->points);

				if (client_order->type == 1) { // sell // сдвиг в 0 надо сделать +3 -1
					if (master_order->slprice != 0 && ((master_order->slprice < SL && master_order->slprice > slprice_max[OP_SELL]) || (master_order->slprice < slprice_min[OP_SELL]))) {
						SL = master_order->slprice;
					}
					else {
						if (client_order->slprice > slprice_min[OP_SELL]) {
							if (!(slprice_min[OP_SELL] && (mpc[client_order->type] - slprice_min[OP_SELL])*sign[client_order->type] <= stoplevel)) {
								SL = slprice_min[OP_SELL];
							}

						}
						else if (abs(client_order->slprice - slprice_min[OP_SELL]) <= digits[(int)Info->digits]) {
							SL = slprice_min[OP_SELL];
						}// иначе стоит максимум безубытка, поэтому ничего не трогаем
					}
				}
				else if (client_order->type == 0) { // buy  //сдвиг -3 +1
					if (master_order->slprice != 0 && ((master_order->slprice > SL && master_order->slprice < slprice_min[OP_BUY]) || (master_order->slprice > slprice_max[OP_BUY]))) {
						SL = master_order->slprice;
					}
					else { // устанавливаем свой SL
						if (client_order->slprice < slprice_max[OP_BUY]) {
							if (!(slprice_max[OP_BUY] && (mpc[client_order->type] - slprice_max[OP_BUY])*sign[client_order->type] <= stoplevel)) {
								SL = slprice_max[OP_BUY];
							}
						}
						else if (abs(client_order->slprice - slprice_max[OP_BUY]) <= digits[(int)Info->digits]) {
								SL = slprice_max[OP_BUY];
						}// иначе стоит максимум безубытка, поэтому ничего не трогаем
					}
				}

				takep = client_order->tpprice;
				if (abs(master_order->tpprice - client_order->tpprice) > digits[(int)Info->digits]) takep = master_order->tpprice;

				if (abs(SL - client_order->slprice) > digits[(int)Info->digits] || abs(takep - client_order->tpprice) > digits[(int)Info->digits]) { // если tp или sl был изменен
					if (!(SL && (mpc[client_order->type] - SL)*sign[client_order->type] <= stoplevel) && client_order->type < 2) {
						modOrder(client_order->ticket, client_order->type, client_order->lots, client_order->openprice, SL, takep, client_order->symbol);
					}
				}*/

				SL = 0;
				takep = 0;
				if (master_order->slprice != 0 || master_order->tpprice != 0) {
					if (master_order->slprice > 0)
						SL = master_order->slprice;
					if (master_order->tpprice > 0)
						takep = master_order->tpprice;
				}
				// копируем стоплосы и тейкпрофит у мастера
				if (abs(SL - client_order->slprice) > digits[(int)Info->digits] || abs(takep - client_order->tpprice) > digits[(int)Info->digits]) { // если tp или sl был изменен
					if (!(SL && (mpc[client_order->type] - SL)*sign[client_order->type] <= stoplevel) && client_order->type < 2) {
						modOrder(client_order->ticket, client_order->type, client_order->lots, client_order->openprice, SL, takep, client_order->symbol);
					}
				}
			}
			if (!found && flag_no_open) { // и все ордера в mql были orderSelect
				bool history_create = false;
				// нужно сделать проверку в закрытых ордерах
				for (int history_index = 0; history_index < ordersCountHistoryMax; history_index++) {
					if (mHistoryTickets[history_index] == master_order->mapedTicket) {
						// ордер найден в истории
						history_create = true;
					}
				}
				if (!history_create && ordersRCount < OPEN_ORDERS_LIMIT) { // ордер не найден, даем команду на открытие ордера, записывать ордер будем при обновлении
					double _lots = 0;
					if (depolot) {
						auto lotsize = msgServer.balance / master_order->lots;
						_lots = balance / depolot;
					}
					else {
						_lots = master_order->lots;
					}
					if (_lots < 2 && _lots > 0 && master_order->closeprice == 0) { // защита от больших ордеров и закрытых ордеров
						createOrder(master_order, _lots);
						//history_update = true;
					}
				}
				else { // тикет был найден в истории, был закрыт скорее всего вручную или по sl / tp, нужно передать закрытие в биллинг
					//std::wcout << "this ticket is closed early - " << ticket_temp << "\r\n";
				}
			}
			master_order = 0;
		}

		for (int client_index = 0; client_index < ordersRCount; client_index++) {
			auto client_order = client_orders + client_index;

			if (client_order->expiration != 1 && msgServer.validation) {
				mHistoryTickets[ordersCountHistory] = getMasterTicket2(client_order->magic);
				ordersCountHistory++;
				if (ordersCountHistory > 499) {
					ordersCountHistoryMax = 500;
					ordersCountHistory = 0;
				}
				else {
					if (ordersCountHistoryMax != 500)
						ordersCountHistoryMax = ordersCountHistory;
				}
				if (client_order->type < 2) {
					closeOrder(client_order);
					newCom = true;
				}
				else {
					deleteOrder(client_order);
					newCom = true;
				}
			}
		}
		//std::wcout << "Receiver actionsCount - " << actionsCount << ". \r\n";
		ordersRCount = 0;
		return actionsCount;
	}


	void ffc_RDeInit() {
		deInitZMQ();
		recieverInit = false;
	}

	//--------------------------------------------------------------------------------------------
}