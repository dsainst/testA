//+------------------------------------------------------------------+
//|                                                  FF_Anderson.mq4 |
//|                        Copyright 2016, BlackSteel, FairForex.org |
//|                                            https://fairforex.org |
//+------------------------------------------------------------------+
#property copyright "Copyright 2016, BlackSteel, FairForex.org"
#property link      "https://fairforex.org"
#property version   "1.00"
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
	string		comment;
};


bool   showinfo;
bool   ShowDebug = false;
int    herror, hddlog;

int    mqlOptimization;


#import "FF_Anderson.dll"
bool ffc_RInit(OrderAction& mql_order_action[], int length, long id_login);
void ffc_RDeInit();
void ffc_ROrdersCount(int orders);
int ffc_RGetJob(int time);
int ffc_ROrdersUpdate(int OrderTicket, int orderMagic, string OrderSymbol, int orderType,
		double OrderLots, double OrderOpenPrice, datetime OrderOpenTime,
		double OrderTakeProfit, double OrderStopLoss, double  OrderClosePrice, datetime  OrderCloseTime,
		datetime OrderExpiration, double  OrderProfit, double  OrderCommission, double  OrderSwap, string OrderComment);
#import
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
#include "Errors.mq4"
#include "ffc.mq4"
#include "Order.mq4"
#define MAX_ORDER_COUNT 200

int totalOrders = 0;

int TimeRestart = 0;
int needUpdate;

OrderAction mql_order_action[MAX_ORDER_COUNT];
  
int OnInit()
  {
      if (!EventSetMillisecondTimer(100)) return(INIT_FAILED);
    
	   mqlOptimization       = IsOptimization();
   
      for (int i=0; i<MAX_ORDER_COUNT; i++) {  //Выделяем память под строку (надо сделать один раз в начале)
         StringInit(mql_order_action[i].symbol, 16);  
         StringInit(mql_order_action[i].comment, 32);
      }
      mql_order_action[0].symbol = "default";
      
       long login = AccountInfoInteger(ACCOUNT_LOGIN);
       
       Alert("login=",login);
      
      if (ffc_RInit(mql_order_action, MAX_ORDER_COUNT, login) != 1) { 
         Print("Повторный запуск!");
         return(INIT_FAILED);
      }
      
      return(INIT_SUCCEEDED);
  }
  
//+------------------------------------------------------------------+
//| Timer function                                                   |
//+------------------------------------------------------------------+
void OnTimer()
  {
   if (!checkMarket()) return;
   
   int minute = Minute();
   needUpdate = minute?minute:60 - TimeRestart;
   
   int ordersTotal = OrdersTotal();
   int ordersCount = 0;
   bool res = false;
   int magicNumber = 0;
   //Alert ("ordersTotal=",ordersTotal);
   while (ordersCount<ordersTotal) {
      if (OrderSelect(ordersCount, SELECT_BY_POS) && (magicNumber=OrderMagicNumber()) > 0) {
          ffc_ROrdersUpdate(OrderTicket(), magicNumber, OrderSymbol(), OrderType(), OrderLots(),
            OrderOpenPrice(), OrderOpenTime(), OrderTakeProfit(), OrderStopLoss(),
            OrderClosePrice(), OrderCloseTime(), OrderExpiration(),
            OrderProfit(), OrderCommission(), OrderSwap(), OrderComment());
            //Print("Symbol=",Symbol());
      }
      ordersCount++;
   }
   
   int action = ffc_RGetJob(needUpdate);
   
   if (action>0) 
	   Alert("action = ",action);
	   
   for (int i = 0; i < action; i++) {
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
		Alert("action=",mql_order_action[i].action," ticket=",mql_order_action[i].ticket," type=",mql_order_action[i].type," kolvo=",action);
	}
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
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick()
  {
  }