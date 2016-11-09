//+------------------------------------------------------------------+
//|                                                        Order.mq4 |
//|                                    Copyright 2016, FairForex.org |
//|                                             http://fairforex.org |
//+------------------------------------------------------------------+
#property library
#property copyright "Copyright 2016, FairForex.org"
#property link      "http://fairforex.org"
#property version   "1.00"
#property strict

int      timeout;

   void CreateOrder(OrderAction& action)
	{
	   if (!checkSymbol(action.symbol)) return; //если нет данных по символу - нечего делать дальше (короткая цепочка выполнения)

	   action.openprice = normPrice(action.openprice, action.symbol); //Нормализуем цену, так как точность котирования у клиента может отличаться от поставщика

	   if (action.type < 2 && ((action.type)? mpo[action.type] < action.openprice : action.openprice > mpo[action.type])) return;  //Если не отложка и цена плохая, ничего не делаем

		int ticket = OrderSend(
			action.symbol,
			action.type, 
			action.lots, 
			mpo[action.type % 2],   //нет смысла нормализовывать цену полученную от терминала, она и так нормализована
			0,
			0,   //тейкпрофит и стоплосс сразу не можем выставить на eсn счетах, выдаст ошибку
			0,
			action.comment,    // а StringConcatenate зачем?
			action.magic,0, clrRed
		);

		if (ticket < 0) {
			int err = GetLastError();
		}
	}

   void ModifyOrder(OrderAction& action)
	{
	   if (!checkSymbol(action.symbol)) return ;
	   if (!isStopLevel(action)) return ;
	   if (!isFreeze(action)) return ;
	   
	   double tpcurrent,slcurrent;
	   if(OrderSelect(action.ticket, SELECT_BY_TICKET)==true)
       {
         tpcurrent = normPrice(OrderTakeProfit(), OrderSymbol());
         slcurrent = normPrice(OrderStopLoss(), OrderSymbol());
         if (normPrice(action.slprice, action.symbol) == slcurrent && normPrice(action.tpprice, action.symbol) == tpcurrent) {
            //Print("Модификация отклонена!");
            return ;
         }
       }
	   Print("команда модификация!", normPrice(action.tpprice, action.symbol), " - ", normPrice(action.slprice, action.symbol));
		//OrderAction a;  <--- а это зачем?
	   //Тут еще надо проверить на уровни Freeze и sl/tp
		if (!OrderModify(action.ticket, action.openprice, action.slprice, action.tpprice, 0, clrBlue)) {
			int err = GetLastError();
			if (err == 1) //Убираем часто встречающуюся ошибку - "Нет ошибки" =)
				return;

		}
	}

   void DeleteOrder(OrderAction& action)
	{
	   if (!checkSymbol(action.symbol)) return ;
		//Тут еще надо проверить на уровни Freeze
	   if (!isFreeze(action)) return ;
		if (!OrderDelete(action.ticket)) {
			int err = GetLastError();
		}
	}

   void CloseOrder(OrderAction& action)
	{
		//Тут еще надо проверить на уровни Freeze
	   if (!checkSymbol(action.symbol)) return ;
	   if (!isFreeze(action)) return ;
		if (!OrderClose(action.ticket, action.lots, normPrice(mpc[action.type], action.symbol), 0, clrGreen)) {
			Alert(GetLastError());
		}
	}

   bool IsPriceOk(OrderAction& action)
   {
	   return ((action.type) ? mpo[action.type] >= action.openprice : action.openprice <= mpo[action.type]);
   }
   
   bool isFreeze(OrderAction& action)
   {
      double freeze = MarketInfo(action.symbol,MODE_FREEZELEVEL);
      // BUY && SELL
      if (freeze == 0) return true;
      //if (action.tpprice>0 && action.slprice>0) return true;
      if ((action.tpprice - mpo[action.type])*sign[action.type]>freeze || (mpo[action.type] - action.slprice)*sign[action.type]>freeze ) return false;
      return true;
   }
   
   
   bool isStopLevel(OrderAction& action)
   {
      double stoplevel = MarketInfo(action.symbol,MODE_STOPLEVEL);
      
      // BUY && SELL
      if (action.tpprice>0 && action.slprice>0) return true;
      if ((action.tpprice - mpo[action.type])*sign[action.type]>stoplevel || (mpo[action.type] - action.slprice)*sign[action.type]>stoplevel ) return false;
      return true;
   }
   
	