#include "ActionManager.h"
#include "utils.h"
#include <iostream>

int ffc::actionsCount = 0;
int ffc::actionsMaxCount = 0;
double ffc::deltaStopLevel = 0;
double ffc::deltaFreezeLevel = 0;
ffc::MqlAction* ffc::actions = NULL;

void ffc::initActions(MqlAction* arrayPtr, int length)
{
	actions = arrayPtr;
	actionsMaxCount = length;
}

void ffc::resetActions() {
	actionsCount = 0;
}

void ffc::createOrder(wchar_t* symbol, int type, double lots, double openPrice, double slPrice, double tpPrice, wchar_t* comment, int stMagic) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_CREATE;
	action->ticket = 0;
	action->type = type;
	action->magic = stMagic;
	action->lots = lots;
	action->openprice = openPrice;
	action->slprice = slPrice;
	action->tpprice = tpPrice;
	action->expiration = 0;

	writeMqlString(action->symbol, symbol);
	//writeMqlString(action->comment, comment);
}
void ffc::createOrder(FfcOrder* order, double _lots) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_CREATE;
	action->ticket = 0;
	action->type = order->type;
	action->magic = order->mapedTicket | MAGIC_EA;
	action->lots = _lots;
	action->openprice = order->openprice;
	action->slprice = order->slprice;
	action->tpprice = order->tpprice;
	action->expiration = 0;

	writeMqlString(action->symbol, order->symbol);
	action->original = order->ticket;
}

void ffc::modOrder(int ticket, int type, double lots, double openprice, double slprice, double tpprice, wchar_t* symbol) { //
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_MODIFY;
	action->ticket = ticket;
	action->type = type;
	action->magic = 0;
	action->lots = lots;
	action->openprice = openprice; // для рыночных ордеров возможно нужен 0 - проверить!!!
	action->slprice = slprice;
	action->tpprice = tpprice;
	action->expiration = 0;

	writeMqlString(action->symbol, symbol);
//	writeMqlString(action->comment, L"");
}

void ffc::deleteOrder(int ticket) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_DELETE;
	action->ticket = ticket;
	action->type = 0;
	action->magic = 0;
	action->lots = 0;
	action->openprice = 0;
	action->slprice = 0;
	action->tpprice = 0;
	action->expiration = 0;

	writeMqlString(action->symbol, L"");
//	writeMqlString(action->comment, L"");
}

void ffc::closeOrder(int ticket, double lots, double openprice) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_CLOSE;
	action->ticket = ticket;
	action->type = 0;
	action->magic = 0;
	action->lots = lots;
	action->openprice = openprice;
	action->slprice = 0;
	action->tpprice = 0;
	action->expiration = 0;

	writeMqlString(action->symbol, L"");
//	writeMqlString(action->comment, L"");
}
void ffc::closeOrder(FfcOrder* order) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_CLOSE;
	action->ticket = order->ticket;
	action->type = order->type;
	action->magic = 0;
	action->lots = order->lots;
	action->openprice = 0;
	action->slprice = 0;
	action->tpprice = 0;
	action->expiration = 0;

	writeMqlString(action->symbol, order->symbol);
//	writeMqlString(action->comment, L"");

	std::wcout << "symbol - " << order->symbol << "\r\n";
}

void ffc::deleteOrder(FfcOrder* order) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_DELETE;
	action->ticket = order->ticket;
}

void ffc::showValue(int line, wchar_t* value) {
	if (actionsCount + 1 >= actionsMaxCount) return;
	auto action = actions + actionsCount;
	actionsCount++;

	action->actionId = JOB_SHOW_VALUE;
	action->ticket = line;
	action->type = 0;
	action->magic = 0;
	action->lots = 0;
	action->openprice = 0;
	action->slprice = 0;
	action->tpprice = 0;
	action->expiration = 0;

	writeMqlString(action->symbol, L"");
//	writeMqlString(action->comment, value);
}


void ffc::terminalInfoCalc(wchar_t* symbol_name) {
	auto Info = &SymbolInfos[WC2MB(symbol_name)];
	deltaStopLevel = Info->stoplevel	* Info->points;
	deltaFreezeLevel = Info->freezelevel	* Info->points;
}

double ffc::normLot(double value, wchar_t* symbol_name) {
	auto Info = &SymbolInfos[WC2MB(symbol_name)];
	value = round(value / Info->lotstep) * Info->lotstep;
	value = max(value, Info->min_lot);
	return min(value, Info->max_lot);
}

double ffc::normLotMin(double value, wchar_t* symbol_name) {
	auto Info = &SymbolInfos[WC2MB(symbol_name)];
	value = floor(value / Info->lotstep) * Info->lotstep;
	value = max(value, Info->min_lot);
	return min(value, Info->max_lot);
}

double ffc::normPrice(double value, wchar_t* symbol_name) {
	auto Info = &SymbolInfos[WC2MB(symbol_name)];
	return floor(value / Info->points + 0.5) * Info->points;
}

