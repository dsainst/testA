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

double POINT;

int newCom;
string out = "";

int      timeout;

   void CreateOrder(OrderAction& action)
	{
	   if (!checkSymbol(action.symbol)) return; //если нет данных по символу - нечего делать дальше (короткая цепочка выполнения)
      
      double digit[6];
      double correct = 10;
      digit[1] = 0.1;
      digit[2] = 0.01;
      digit[3] = 0.001;
      digit[4] = 0.0001;
      digit[5] = 0.00001;
      
      int index = (int)MarketInfo(action.symbol, MODE_DIGITS);
      
      /*if (index>3) // коррекция только для 4 и 5 знаковых
      correct = correct*digit[index]*sign[action.type]*(-1); else */
      correct = 0;
	   action.openprice = normPrice(action.openprice, action.symbol); //Нормализуем цену, так как точность котирования у клиента может отличаться от поставщика

	   if ((action.type)? action.openprice - mpo[action.type] > digit[index]*3 : mpo[action.type] - action.openprice > digit[index]*3) {
   	   if (debuggerActive) Alert("Цена плохая = ", mpo[action.type], " - ",action.openprice, "action type = ",action.type, " action lot = ",normLot(action.lots, action.symbol));
   	   string comment = StringConcatenate("limit_",action.original);
   	   int ticket = OrderSend( // выставляем отложенный ордер
   			action.symbol,
   			action.type+2, 
   			normLot(action.lots, action.symbol), 
   			action.openprice+correct,
   			15,
   			0,   //тейкпрофит и стоплосс сразу не можем выставить на eсn счетах, выдаст ошибку
   			0,
   			comment,    // а StringConcatenate зачем?
   			action.magic,0, clrBlack
   		);
   		
   	   out = StringConcatenate("команда открытие отложки!\r\nopen_price=", action.openprice, " - symbol= ", action.symbol, " - type= ", action.type+2, " - lot= ", action.lots, " - magic= ", action.magic);
   	   logFile(out);
   	   
   		if (ticket < 0) {
			   logFile((string)GetLastError());
   			int err = GetLastError();
   		   if (debuggerActive) Alert("Order send error = ", err);
   		} else {
   		// тикет получили
   		   if (debuggerActive) Alert("Открылся тикет = ", ticket);
   		   if (OrderSelect(ticket, SELECT_BY_TICKET)) {
   		   out = StringConcatenate("добавляем в историю тикет =", ticket);
   	      logFile(out);
   		      ffc_RHistoryUpdate(OrderMagicNumber(), ticket, OrderCloseTime(), OrderClosePrice(), 
                                    OrderType(), OrderSymbol(),OrderLots(),OrderOpenPrice(),OrderOpenTime(),
                                    OrderTakeProfit(),OrderStopLoss(),OrderSwap(),OrderCommission(),OrderProfit()
                                  );
                
            	if (showinfo) {
            		info.ResizeBox();
            		info.DrawLine(
            			IntegerToString(OrderOpenTime()) + "-" + IntegerToString(OrderType()),
            			OrderOpenTime(), OrderOpenPrice(),
            			OrderCloseTime(), OrderClosePrice(),
            			clrRed);
            		info.DelOldDraws();
            	}
           }
   		}
   	   return;  //Если не отложка и цена плохая, ничего не делаем
	   }
	   if (debuggerActive) Alert("Цена хорошая = ", mpo[action.type], " - ",action.openprice, "action lots = ",normLot(action.lots, action.symbol));
	   //action.openprice = (action.type)?fmax(action.openprice,mpo[action.type]):fmin(action.openprice,mpo[action.type]);
	   action.openprice = mpo[action.type];
		int ticket = OrderSend(
			action.symbol,
			action.type, 
			normLot(action.lots, action.symbol), 
			action.openprice+correct,
			//mpo[action.type % 2],   //нет смысла нормализовывать цену полученную от терминала, она и так нормализована
			15,
			0,   //тейкпрофит и стоплосс сразу не можем выставить на eсn счетах, выдаст ошибку
			0,
			(string)action.original,    // а StringConcatenate зачем?
			action.magic,0, clrRed
		);
		
	   out = StringConcatenate("команда открытие ордера!\r\nopen_price=", action.openprice, " - symbol= ", action.symbol, " - type= ", action.type+2, " - lot= ", action.lots, " - magic= ", action.magic);
	   logFile(out);

		if (ticket < 0) {
			logFile((string)GetLastError());
			int err = GetLastError();
		   if (debuggerActive) Alert("Order send error = ", err);
		} else {
		// тикет получили
		   Print("Открылся тикет = ", ticket);
		   if (OrderSelect(ticket, SELECT_BY_TICKET)) {
   		   out = StringConcatenate("добавляем в историю тикет =", ticket);
   	      logFile(out);
		      ffc_RHistoryUpdate(OrderMagicNumber(), ticket, OrderCloseTime(), OrderClosePrice(), 
                                 OrderType(), OrderSymbol(),OrderLots(),OrderOpenPrice(),OrderOpenTime(),
                                 OrderTakeProfit(),OrderStopLoss(),OrderSwap(),OrderCommission(),OrderProfit()
                               );
               ShowInfoWindow(StringConcatenate("Open Ticket ",ticket), OrderSymbol());
        } else {
            ffc_RHistoryUpdate(action.magic,ticket, 0,0,0,"",0,0,0,0,0,0,0,0);
        }
		}
	}

   void ModifyOrder(OrderAction& action)
	{
	   if (!checkSymbol(action.symbol)) return ;
	   if (!isStopLevel(action)) {
   	   if (debuggerActive) Alert("isStopLevelBAD ",action.slprice);
   	   return ;
	   }
	   
	   double tpcurrent,slcurrent;
	   if(!OrderSelect(action.ticket, SELECT_BY_TICKET)) {
	      Print ("Order can`t Select order");
	      return ;
	   }
	   //if (!isEditSL(action, OrderStopLoss())) return;
	   
	   if (debuggerActive) Alert("Modify Order is start!");
      if (!isFreeze(action)) {
   	   if (debuggerActive) Alert("isFreeze");
	      return ;
	   }
      tpcurrent = normPrice(OrderTakeProfit(), OrderSymbol());
      slcurrent = normPrice(OrderStopLoss(), OrderSymbol());
      if (normPrice(action.slprice, action.symbol) == slcurrent && normPrice(action.tpprice, action.symbol) == tpcurrent) {
         Print("Модификация отклонена! ticket=", action.ticket, " slcurrent = ", slcurrent, " actionsl = ", action.slprice, " digits = ", SymbolInfoInteger(action.symbol, SYMBOL_DIGITS));
         return ;
      }
	   Print("команда модификация!", normPrice(action.tpprice, action.symbol), " - ", action.slprice);
	   out = StringConcatenate("команда модификация!\r\nticket=", action.ticket," - tpprice=", action.tpprice, " - symbol= ", action.symbol, " - slprice= ", action.slprice, " - type= ", action.type, " - lot= ", action.lots, " - magic= ", action.magic);
	   logFile(out);
		if (!OrderModify(action.ticket, action.openprice, action.slprice, action.tpprice, 0, clrBlue)) {
			logFile((string)GetLastError());
			int err = GetLastError();
			if (err == 1) //Убираем часто встречающуюся ошибку - "Нет ошибки" =)
				return;

		} else
		newCom = action.ticket;
	}

   void DeleteOrder(OrderAction& action)
	{
	   if (!checkSymbol(action.symbol)) return ;
		//Тут еще надо проверить на уровни Freeze
	   if(!OrderSelect(action.ticket, SELECT_BY_TICKET)) return ;
	   if (!isFreeze(action)) return ;
		if (!OrderDelete(action.ticket)) {
			logFile((string)GetLastError());
			int err = GetLastError();
		}
	   out = StringConcatenate("команда удаление отложки!\r\nticket=", action.ticket, " - symbol= ", action.symbol, " - openprice= ", action.openprice, " - type= ", action.type, " - lot= ", action.lots, " - magic= ", action.magic);
	   logFile(out);
	}

   void CloseOrder(OrderAction& action)
	{
		//Тут еще надо проверить на уровни Freeze
	   if (!checkSymbol(action.symbol)) return ;
	   if(!OrderSelect(action.ticket, SELECT_BY_TICKET)) return ;
	   if (!isFreeze(action)) return ;
	   if (debuggerActive) Alert("action lots - ",OrderLots(),"action lots2 - ",action.lots);
	   out = StringConcatenate("команда закрытие ордера!\r\nticket=", action.ticket, " - symbol= ", action.symbol, " - openprice= ", action.openprice, " - type= ", action.type, " - lot= ", action.lots, " - magic= ", action.magic);
	   logFile(out);
		if (!OrderClose(action.ticket, action.lots, normPrice(mpc[action.type], action.symbol), 0, clrGreen)) {
			logFile((string)GetLastError());
			if (debuggerActive) Alert(GetLastError());
		}
		newCom = action.ticket;
		
	}
	
	void CheckClosedOrders(int ticket) {
		if (OrderSelect(ticket, SELECT_BY_TICKET) == true ) {
		   datetime closeTime = OrderCloseTime();
		   if ((string)closeTime != "1970.01.01 00:00:00" ) {
            ffc_RClosedUpdate(OrderMagicNumber(), OrderTicket(), closeTime, OrderClosePrice(), OrderType(), OrderSymbol(),OrderLots(),OrderOpenPrice(),OrderOpenTime(),OrderTakeProfit(),OrderStopLoss(),OrderSwap(),OrderCommission(),OrderProfit());
            Print("Передаем инфо о закрытом ордере = ",ticket);
         }
      }
   }

   bool IsPriceOk(OrderAction& action)
   {
	   return ((action.type) ? mpo[action.type] >= action.openprice : action.openprice <= mpo[action.type]);
   }
   
   // требуется OrderSelect()
   bool isFreeze(OrderAction& action)
   {
      double freeze = MarketInfo(action.symbol,MODE_FREEZELEVEL) * symbolPoint;
      // BUY && SELL
      if (freeze == 0) return true;
      if (fabs(OrderTakeProfit() - mpc[action.type])<=freeze || fabs(OrderStopLoss() - mpc[action.type])<=freeze) return false;
      return true;
   }
   
   
   bool isStopLevel(OrderAction& action)
   {
      double stoplevel = MarketInfo(action.symbol,MODE_STOPLEVEL) * symbolPoint;
      
      if ((action.tpprice && (action.tpprice - mpc[action.type])*sign[action.type]<=stoplevel) || (action.slprice && (mpc[action.type] - action.slprice)*sign[action.type]<=stoplevel) ) return false;
      return true;
   }
   
   
  /* bool isEditSL(OrderAction& action, double slprice)
   {
      symbolT = action.symbol;
      stoploss = action.slprice; // переданная мастер-slprice; slprice - текущая slprice; sl - рассчитанная 2% slprice
      tickvalue = MarketInfo(action.symbol, MODE_TICKVALUE);
      deltaSL = (AccountBalance()*procent) / (tickvalue*action.lots);
      openPrice = action.openprice;
      double pointer = MarketInfo(symbolT, MODE_POINT);
      POINT = 4*pointer;
      SL = (action.type)? openPrice+(deltaSL*pointer) : openPrice-(deltaSL*pointer);
      switch (action.type) {
         case OP_BUY: //BUY
            if ((stoploss-SL)>POINT) { // зона free
               if (fabs(stoploss-slprice) > POINT) {
                  action.slprice = stoploss;
               } else if (stoploss == 0) {
                  action.slprice = SL;
               } else return false;
            } else if ((stoploss-SL) < 0 || fabs(stoploss-SL) <= POINT || stoploss == 0) { // зона 2
               if (fabs(slprice - SL)<=POINT) {
                  return false;
               } else {
                  action.slprice = SL;
               }
            }
            return true;
            break;
         case OP_SELL: // SELL
            if ((SL - stoploss)>POINT) { // зона free
            Alert("asda");
               if (fabs(slprice-stoploss) > POINT  && stoploss != 0) {
            Alert("asda2");
                  action.slprice = stoploss;
               } else if (stoploss == 0 && fabs(slprice - SL)> POINT) {
            Alert("asda3");
                  action.slprice = SL;
               } else return false;
            } else if ((SL - stoploss) < 0 || fabs(stoploss-SL) <= POINT || stoploss == 0) { // зона 2
               if (fabs(slprice - SL)<=POINT) {
            Alert("asda5");
                  return false;
               } else {
            Alert("asda7");
                  action.slprice = SL;
               }
            }
            return true;
            break;
      }
      
      Alert("Что то пошло не так! Order.mq4");
      return false;
      /*
      if(fabs(slprice-SL)<POINT && stoploss==0) return false;
      bool tvalue = (action.type)? (stoploss<SL && fabs(slprice-stoploss)>POINT) : (stoploss>SL && fabs(slprice-stoploss)>POINT);
      action.slprice = (tvalue)? stoploss : SL;
      if (tvalue) return true;
      bool neravno = (fabs(slprice-SL)>POINT)? true : false; // ничего не делаем
      
      bool tvalue3 = (action.type)? slprice>SL : slprice<SL;
      
      
      Alert("value1=",tvalue," value2=",neravno," value3=",tvalue3);
      Alert("SL=",SL," stoploss=",stoploss," slprice=",slprice);
      if (!neravno) return false;
      if (tvalue3) { action.slprice = SL;}
      return true;
      
   }
	*/