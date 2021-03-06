//+------------------------------------------------------------------+
//|                                                  FF_Anderson.mq4 |
//|                        Copyright 2016, BlackSteel, FairForex.org |
//|                                            https://fairforex.org |
//+------------------------------------------------------------------+
#property copyright "Copyright 2016, BlackSteel, FairForex.org"
#property link      "https://fairforex.org"
#property version   "1.3"
#property strict

#define JOB_EXIT        0
#define JOB_CREATE      1
#define JOB_MODIFY      2
#define JOB_DELETE      3
#define JOB_CLOSE       4
#define JOB_PRINT_ORDER 5
#define JOB_PRINT_TEXT  6
#define JOB_SOFT_BREAK  7
#define JOB_SHOW_VALUE  8
#define JOB_MSG_BOX     9
#define JOB_CHECK       10

#define PROJECT_URL     "fairforex.org"
#define EXPERT_NAME     "FF_Anderson"
#define EXPERT_VERSION  "1.3b"

struct OrderAction
{
	int			action;
	int			ticket;
	int			type;
	int			magic;
	string		symbol;
	double		lots;
	double		openprice;
	double		tpprice;
	double		slprice;
	datetime	   expiration;
	//string      comment;
	int         original;
};


bool   showinfo;
bool   ShowDebug = false;
int    herror, hddlog;
//Аквтивируем все алерты
bool debuggerActive = 0;
int    mqlOptimization;
int    mqlTester;
int    mqlVisualMode;
string symbolName;
double symbolLotStep;
double symbolMarginRequired;
double kdpi;
color  colors[7] = {clrDodgerBlue, clrRed, clrNONE, clrNONE, clrNONE, clrNONE, clrWhite};
string lotnames[2];

bool   show_cpanel = false;
double BaseSellLot = 0.01;
double BaseBuyLot = 0.01;
int DrawHistoryDays = 2;


#import "FF_Anderson.dll"
bool ffc_RInit(OrderAction& mql_order_action[], int length, double risk_percent, int depolot, long id_login, int flag_reinit);
void ffc_RDeInit();
bool ffc_Rworkout();
void ffc_RSetParam(double balance, double equity, double profit, int tradeMode, double Rfreemargin, int Rleverage, int Rlimit, int Rstoplevel, int Rstopmode, string currency, string company, string name, string server, string email);
void ffc_ROrdersCount(int orders);
int ffc_RGetJob(bool flag);
int ffc_RInterestTickets();
int ffc_ROrdersUpdate(int orderTicket, int OrderMagic, string orderSymbol, int orderType,
		double orderLots, double orderPrice, datetime orderTime, double orderTakeProfit, double orderStopLoss,
		datetime orderExpiration, double  tickvalue, double point, double ask, double bid);
void ffc_RHistoryUpdate(int orderMagic, int orderTicket, datetime orderTime, double orderPrice, int orderType, string orderSymbol, double orderLots, double orderOpenPrice, datetime orderOpenTime, double orderTp, double orderSl, double orderSwap, double orderCom, double orderProf);
void ffc_RClosedUpdate(int orderMagic, int orderTicket, datetime orderTime, double orderPrice, int orderType, string orderSymbol, double orderLots, double orderOpenPrice, datetime orderOpenTime, double orderTp, double orderSl, double orderSwap, double orderCom, double orderProf);
void ffc_RInitSymbols(string name, double min_lot, double max_lot, double lotstep, 
                     double tickvalue, double point, double lotsize, 
                     double digits, double stoplevel, double freezelevel, double trade_allowed);
int ffc_getDpi();
#import
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
#include "Look.mq4"
#include "ffc.mq4"
#include "Order.mq4"
#define MAX_ORDER_COUNT 200

int totalOrders = 0;
int disconnect = 0;
Look info(10, clrWhite, 10, 16);

OrderAction mql_order_action[MAX_ORDER_COUNT];

//--- input parameters
extern double RISK_PERCENT = 0.02; // Risk percent of balance
//--- day of week
enum lotSize
  {
   NORMAL=0,
   SCALING=1
  };
//--- input parameters
input lotSize DEPO_LOT=SCALING; // Lot

   double SL = 0;
   double deltaSL = 0;
   double tickvalue = 0;
   double openPrice = 0;
   double point = 0;
   double stoploss = 0;
   string symbolT = "";
   

extern string YOUR_INFO_EMAIL = ""; // Your email for news


int OnInit()
  { // нужна проверка на загрузку терминала
      long login = AccountInfoInteger(ACCOUNT_LOGIN);
      
      initParam();

      if (ffc_RInit(mql_order_action, MAX_ORDER_COUNT, RISK_PERCENT, DEPO_LOT, login, 0) != 1) { 
         Print("Повторный запуск!");
         return(INIT_FAILED);
      }
      
   	mqlOptimization       = IsOptimization();
   	mqlTester             = IsTesting();
   	mqlVisualMode         = IsVisualMode();
   	symbolName            = Symbol();
   	symbolLotStep         = MarketInfo(symbolName, MODE_LOTSTEP);
   	symbolMarginRequired  = MarketInfo(symbolName, MODE_MARGINREQUIRED);
	   
	   showinfo    = (mqlOptimization || (mqlTester && !mqlVisualMode)) ? false : true; 
	   if (showinfo) InitLook();
   
      for (int i=0; i<MAX_ORDER_COUNT; i++) {  //Выделяем память под строку (надо сделать один раз в начале)
         StringInit(mql_order_action[i].symbol, 16);  
         //StringInit(mql_order_action[i].comment, 32);
      }
      mql_order_action[0].symbol = "default";
       
      ShowInfoWindow("Update history tickets...");
      // обновим массив с уже открытыми ордерами
      updateHistoryOrders();
      
      ShowInfoWindow("Start update symbols...");
      
      InitSymbols();
      
      if (!EventSetMillisecondTimer(100)) return(INIT_FAILED);
      
	   ShowInfoWindow("Working...");
      return(INIT_SUCCEEDED);
  }
  
  void InitSymbols () {
   for (int pos=0; pos<SymbolsTotal(false); pos++) {
      string name = SymbolName(pos, false);
      if (StringFind(name,"#") == -1 && StringFind(name,"Sell") == -1 && StringFind(name,"Orders") == -1 && StringFind(name,"Buy") == -1 && StringFind(name,"Estimated") == -1 && StringFind(name,"Trades") == -1 && StringFind(name,"Volume") == -1 && StringFind(name,"DEPTH") == -1  && StringFind(name,"GOLD") == -1 && StringFind(name,"SILVER") == -1)
      {
         SymbolSelect(name, true);
         double min_lot = MarketInfo(name, MODE_MINLOT);
         double max_lot = MarketInfo(name, MODE_MAXLOT);
         double lotstep = MarketInfo(name, MODE_LOTSTEP);
         double tick_value = MarketInfo(name, MODE_TICKVALUE);
         double points = MarketInfo(name, MODE_POINT);
         double lotsize = MarketInfo(name, MODE_LOTSIZE);
         double digits = MarketInfo(name, MODE_DIGITS);
         double stoplevel = MarketInfo(name, MODE_STOPLEVEL);
         double freezelevel = MarketInfo(name, MODE_FREEZELEVEL);
         double trade_allowed = MarketInfo(name, MODE_TRADEALLOWED);
         ffc_RInitSymbols(name, min_lot, max_lot, lotstep, tick_value, points, lotsize, digits, stoplevel, freezelevel, trade_allowed);
      }
   }
  }
  
//+------------------------------------------------------------------+
//| Timer function                                                   |
//+------------------------------------------------------------------+
void OnTimer()
  {
  // проверка почты zmq
  if(!IsConnected())
    {
    disconnect = 1;
	   ShowInfoWindow("Please, check internet connection!");
     Print("Подключение к интернету отсутствует!");
     return;
    } else {
      if (disconnect) {
   	   ShowInfoWindow("Reinit...");
         Print("Связь появилась, инициализация");
         reInit();
         disconnect = 0;
      }
    }
    if (!TerminalInfoInteger(TERMINAL_TRADE_ALLOWED)) {
      return;
    }
   if (!checkMarket()) return;
   if (ffc_Rworkout()) return;
   
   int ordersTotal = OrdersTotal();
   int ordersCount = 0;
   int magicNumber = 0;
   double ask = 0;
   double bid = 0;
   string symb = "";
   bool orderOpenAccess = true;
   
   initParam();
   
   //ShowInfoWindow(StringConcatenate("Orders total:  ",ordersTotal));
   
   while (ordersCount<ordersTotal) {
      if (OrderSelect(ordersCount, SELECT_BY_POS) && (magicNumber=OrderMagicNumber()) > 0) {
            symb = OrderSymbol();
            point = MarketInfo(symb, MODE_POINT);
            tickvalue = MarketInfo(symb, MODE_TICKVALUE);
            ask = MarketInfo(symb, MODE_ASK);
            bid = MarketInfo(symb, MODE_BID);
            int accAllowed = 0;
            if (magicNumber>1000)
               accAllowed = ffc_ROrdersUpdate(OrderTicket(), magicNumber, symb, OrderType(), OrderLots(),
                                           OrderOpenPrice(), OrderOpenTime(), OrderTakeProfit(), 
                                           OrderStopLoss(), OrderExpiration(), tickvalue, point, ask, bid);
      } else {
		// flag no_open
		orderOpenAccess = false;
	  }
      ordersCount++;
   }
   
   int action = ffc_RGetJob(orderOpenAccess);
   
   if (action>0 && debuggerActive) 
	   Alert("action = ",action);
	 
   for (int i = 0; i < action; i++) {
      newCom = 0;
		switch (mql_order_action[i].action) {
			case JOB_CREATE: CreateOrder    (mql_order_action[i]); break;
			case JOB_MODIFY: ModifyOrder    (mql_order_action[i]); break;
			case JOB_DELETE: DeleteOrder    (mql_order_action[i]); break;
			case JOB_CLOSE: CloseOrder     (mql_order_action[i]); break;
			case 5: break;
			case 6: ShowInfoWindow("All OK!"); break;
			case 7: ShowInfoWindow("You are not our partner!"); break;
			case 8: break;
			case 9: break;
			case JOB_CHECK: CheckClosedOrders(mql_order_action[i].ticket); break;
		}
		// если закрытие, то передаем этот ордер в биллинг
		if (newCom) CheckClosedOrders(newCom);
		if (debuggerActive) Alert("action=",mql_order_action[i].action," ticket=",mql_order_action[i].ticket," type=",mql_order_action[i].type," kolvo=",action);
	}
	
	// для тикетов закрытых по sl/tp и вручную
	//int getTicket = ffc_RInterestTickets();
   //if (getTicket) CheckClosedOrders(getTicket);
	
	
}

void reInit() {
   long login = AccountInfoInteger(ACCOUNT_LOGIN);
   
   initParam();

   if (ffc_RInit(mql_order_action, MAX_ORDER_COUNT, RISK_PERCENT, DEPO_LOT, login, 1) != 1) { 
      Print("Повторный запуск!");
      EventKillTimer();
   }
   ShowInfoWindow("working...");
}

int initParam() {
   ffc_RSetParam(AccountBalance(), AccountEquity(), AccountProfit(), (int)AccountInfoInteger(ACCOUNT_TRADE_MODE), AccountFreeMarginMode(), AccountLeverage(), (int)AccountInfoInteger(ACCOUNT_LIMIT_ORDERS), AccountStopoutLevel(), AccountStopoutMode(), AccountCurrency(), AccountCompany(),AccountName(),AccountServer(), YOUR_INFO_EMAIL);
   return true;
}


//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
   Print("Deinit");
   EventKillTimer();
   ffc_RDeInit();
  }
  
void updateHistoryOrders() {
      int magicNumber;
      int accTotal=OrdersHistoryTotal();
      int ordersTotalHistory = 100;
      int ordersCountHistory = 0;
      int ordersCount = 0;
      if (accTotal>100) {
         ordersTotalHistory = accTotal;
         ordersCountHistory = accTotal - 100;
      }
      while (ordersCountHistory<=ordersTotalHistory) {
         if (OrderSelect(ordersCountHistory, SELECT_BY_POS, MODE_HISTORY) && (magicNumber=OrderMagicNumber()) > 1000) {
            ffc_RHistoryUpdate( OrderMagicNumber(), OrderTicket(), OrderCloseTime(), OrderClosePrice(), 
                                 OrderType(), OrderSymbol(),OrderLots(),OrderOpenPrice(),OrderOpenTime(),
                                 OrderTakeProfit(),OrderStopLoss(),OrderSwap(),OrderCommission(),OrderProfit()
                               );
         }
         ordersCountHistory++;
      }
      // открытые ордера тоже заносим в этот массив, защита от двойного открытия
      int ordersTotal = OrdersTotal();
      while (ordersCount<ordersTotal) {
         if (OrderSelect(ordersCount, SELECT_BY_POS) && (magicNumber=OrderMagicNumber()) > 1000) {
               ffc_RHistoryUpdate( OrderMagicNumber(), OrderTicket(), OrderCloseTime(), OrderClosePrice(), 
                                 OrderType(), OrderSymbol(),OrderLots(),OrderOpenPrice(),OrderOpenTime(),
                                 OrderTakeProfit(),OrderStopLoss(),OrderSwap(),OrderCommission(),OrderProfit()
                               );
         }
         ordersCount++;
      }
}

void InitLook()
{
	kdpi = ffc_getDpi()/72.0;
	ObjectsDeleteAll();

	info.Init();
	info.SetHeader(0, PROJECT_URL);
	info.SetHeader(1, StringConcatenate(EXPERT_NAME," v", EXPERT_VERSION));
	info.Set("Waiting for first tick...");
	
	//ShowControlPanel(info.x + info.dx + 10, info.y);
	info.DrawHistory();
	info.ResizeBox();
}

void ShowInfoWindow(string msg, string symbol="EURUSD")
{
   if (showinfo) {
      string ask = (string)MarketInfo(Symbol(), MODE_ASK);
      string bid = (string)MarketInfo(Symbol(), MODE_BID);
      info.Set(msg);
      info.Set(StringConcatenate("currency         ",Symbol()));
      info.Set(StringConcatenate("ask              ",ask));
      info.Set(StringConcatenate("bid              ",bid));
      
      if (!TerminalInfoInteger(TERMINAL_DLLS_ALLOWED)) 
         info.Set("Import dll is not allowed"); else
         info.Set("Dll is allowed!");
         
      if (!TerminalInfoInteger(TERMINAL_TRADE_ALLOWED)) {
         info.Set("auto-trade not allowed");
         info.Set("press auto-trade button");
         } else
         info.Set("auto-trade is allowed!");
      info.ResizeBox();
	}
}