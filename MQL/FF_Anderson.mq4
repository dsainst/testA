//+------------------------------------------------------------------+
//|                                                  FF_Anderson.mq4 |
//|                        Copyright 2016, BlackSteel, FairForex.org |
//|                                            https://fairforex.org |
//+------------------------------------------------------------------+
#property copyright "Copyright 2016, BlackSteel, FairForex.org"
#property link      "https://fairforex.org"
#property version   "1.01"
#property strict

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
};


bool   showinfo;
bool   ShowDebug = false;
int    herror, hddlog;
//Аквтивируем все алерты
bool debuggerActive = 0;
int    mqlOptimization;


#import "FF_Anderson.dll"
bool ffc_RInit(OrderAction& mql_order_action[], int length, double risk_percent, int depolot, long id_login, string fullpath);
void ffc_RDeInit();
bool ffc_Rworkout();
void ffc_RSetParam(double balance, double equity, double profit, int tradeMode, double Rfreemargin, int Rleverage, int Rlimit, int Rstoplevel, int Rstopmode, string currency, string company, string name, string server);
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
#import
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
#include "Errors.mq4"
#include "ffc.mq4"
#include "Order.mq4"
//#include "SymbolsInfo.mq4"
#define MAX_ORDER_COUNT 200

int totalOrders = 0;

OrderAction mql_order_action[MAX_ORDER_COUNT];

//--- input parameters
extern double RISK_PERCENT = 0.02;
//--- day of week
enum lotSize
  {
   NORMAL=0,
   SCALING=1
  };
//--- input parameters
input lotSize DEPO_LOT=SCALING;

   double SL = 0;
   double deltaSL = 0;
   double tickvalue = 0;
   double openPrice = 0;
   double point = 0;
   double stoploss = 0;
   string symbolT = "";
   
   


int OnInit()
  {
      long login = AccountInfoInteger(ACCOUNT_LOGIN);
      
      initParam();

      if (ffc_RInit(mql_order_action, MAX_ORDER_COUNT, RISK_PERCENT, DEPO_LOT, login, fullpath) != 1) { 
         Print("Повторный запуск!");
         return(INIT_FAILED);
      }
      
	   mqlOptimization       = IsOptimization();
   
      for (int i=0; i<MAX_ORDER_COUNT; i++) {  //Выделяем память под строку (надо сделать один раз в начале)
         StringInit(mql_order_action[i].symbol, 16);  
         //StringInit(mql_order_action[i].comment, 32);
      }
      mql_order_action[0].symbol = "default";
       
      //Alert("login=",login);
      
      
      int ordersTotalHistory = 100;
      int magicNumber;
      int ordersCountHistory = 0;
      while (ordersCountHistory<ordersTotalHistory) {
         if (OrderSelect(ordersCountHistory, SELECT_BY_POS, MODE_HISTORY) && (magicNumber=OrderMagicNumber()) > 0) {
             ffc_RHistoryUpdate( OrderMagicNumber(), OrderTicket(), OrderCloseTime(), OrderClosePrice(), 
                                 OrderType(), OrderSymbol(),OrderLots(),OrderOpenPrice(),OrderOpenTime(),
                                 OrderTakeProfit(),OrderStopLoss(),OrderSwap(),OrderCommission(),OrderProfit()
                               );
         }
         ordersCountHistory++;
      }
      
      InitSymbols();
      
      if (!EventSetMillisecondTimer(100)) return(INIT_FAILED);
      
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
   
   while (ordersCount<ordersTotal) {
      if (OrderSelect(ordersCount, SELECT_BY_POS) && (magicNumber=OrderMagicNumber()) > 0) {
            symb = OrderSymbol();
            point = MarketInfo(symb, MODE_POINT);
            tickvalue = MarketInfo(symb, MODE_TICKVALUE);
            ask = MarketInfo(symb, MODE_ASK);
            bid = MarketInfo(symb, MODE_BID);
          ffc_ROrdersUpdate(OrderTicket(), magicNumber, symb, OrderType(), OrderLots(),
            OrderOpenPrice(), OrderOpenTime(), OrderTakeProfit(), OrderStopLoss(), OrderExpiration(), tickvalue, point, ask, bid);
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
			case 6: break;
			case 7: break;
			case 8: break;
			case 9: break;
			case 10: break;
		}
		if (newCom) CheckClosedOrders(newCom);
		if (debuggerActive) Alert("action=",mql_order_action[i].action," ticket=",mql_order_action[i].ticket," type=",mql_order_action[i].type," kolvo=",action);
	}
	
	int getTicket = ffc_RInterestTickets();
   if (getTicket) CheckClosedOrders(getTicket);
	
	
}

int initParam() {
   ffc_RSetParam(AccountBalance(), AccountEquity(), AccountProfit(), (int)AccountInfoInteger(ACCOUNT_TRADE_MODE), AccountFreeMarginMode(), AccountLeverage(), (int)AccountInfoInteger(ACCOUNT_LIMIT_ORDERS), AccountStopoutLevel(), AccountStopoutMode(), AccountCurrency(), AccountCompany(),AccountName(),AccountServer());
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