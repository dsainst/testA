//+------------------------------------------------------------------+
//|                                                          ffc.mq4 |
//|                                    Copyright 2016, FairForex.org |
//|                                             http://fairforex.org |
//+------------------------------------------------------------------+
#property library
#property copyright "Copyright 2016, FairForex.org"
#property link      "http://fairforex.org"
#property version   "1.00"
#property strict

MqlTick symbol_tick;
double mpo[2];
double mpc[2];
int    sign[2] = {1,-1};
double kurs = 1; 
double value;
double symbolPoint;

const string filename = "log_Anderson.txt";
const string path = StringConcatenate(TerminalInfoString(TERMINAL_DATA_PATH), "/MQL4/Files/");
const string fullpath = StringConcatenate(path,filename);


//Нормализует лот с учетом минимального значения и допустимого шага
double normLot(double lots, string symbol) {
   double lot_step = SymbolInfoDouble(symbol, SYMBOL_VOLUME_STEP);
   int steps = int(round(lots/lot_step));
   lots = steps * lot_step;
   lots = MathMax(lots, SymbolInfoDouble(symbol, SYMBOL_VOLUME_MIN));
   return(MathMin(lots, SymbolInfoDouble(symbol, SYMBOL_VOLUME_MAX)));
}
//Нормализует цену
double normPrice(double price, string symbol)
{
   return(NormalizeDouble(price, (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS)));
}
//нормализует размер депозита под условия как на местер счете
double normDepo(string symbol) {
   return(AccountBalance() * 
      (100000/SymbolInfoDouble(symbol, SYMBOL_TRADE_CONTRACT_SIZE)) *
      (SymbolInfoDouble(symbol, SYMBOL_VOLUME_MIN)/0.01) *
      kurs
   );
}
//Проверяем возможность работы по символу, и обновляем структуру symbol_tick
bool checkSymbol(string symbol) {
   if (!SymbolSelect(symbol, true)) return false;  //Пытаемся выбрать символ, если символа нет,
   if (!(bool)MarketInfo(symbol, MODE_TRADEALLOWED)) return false;  //проверяем, разрешена ли торговля по символу
   if (!SymbolInfoTick(symbol, symbol_tick)) return false;  //получаем текущую информацию по символу (надо проверить, нужнен ли маркет рефреш)
   mpo[OP_BUY]  = symbol_tick.ask;
   mpo[OP_SELL] = symbol_tick.bid;
   mpc[OP_BUY]  = symbol_tick.bid;
   mpc[OP_SELL] = symbol_tick.ask;
   symbolPoint = MarketInfo(symbol, MODE_POINT);
   
   return true; 
}

bool checkMarket() {
   return (IsConnected() && !IsTradeContextBusy() && IsTradeAllowed());
}

bool logFile(string line) {
   ResetLastError();
   int handle=FileOpen(filename,FILE_WRITE|FILE_READ|FILE_TXT);
   if(handle!=INVALID_HANDLE)
     {
      FileSeek(handle, 0, SEEK_END);
      FileWrite(handle,TimeToStr(TimeCurrent(),TIME_DATE|TIME_SECONDS),"\r\n",line, "\r\n----------------------------------\r\n");
      FileClose(handle);
      return true;
     }
   else
      Print("Failed to open the file, error ",GetLastError());
   return false;
}