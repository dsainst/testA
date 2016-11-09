#include "string.h"
#include "utils.h"

#include "zmq.h"

#include <assert.h>
#include <stdlib.h>
#include <thread>
#include <atomic>
#include <iostream>

#define SERVER_ADDR "tcp://212.116.110.46:8084"
//#define SERVER_ADDR "tcp://127.0.0.1:8083"

void* context;
void* request;
int ffc::validOrder;
int ffc::updateOrder;

int ffc::getMasterTicket(wchar_t* comment) {
	wchar_t* pwc;
	pwc = wcstok(comment, L"_");
	pwc = wcstok(NULL, L"_");
	if (pwc == NULL) return 0;
	//std::wcout << "getMagic " << comment << "/" << _wtoi(pwc) << "\r\n";
	return _wtoi(pwc);
}

///Копируем строку с++ в MQLSting (на стороне MQL больше ничего делать не надо)
void ffc::writeMqlString(MqlString dest, wchar_t* source) {
	int len = min(wcslen(source), dest.size - 1);  //Определяем длину строки (небольше распределенного буфера)
	wcscpy_s(dest.buffer, len + 1, source);  //Копируем включая терминирующий ноль
	*(((int*)dest.buffer) - 1) = len;  // Записываем длину строки (хак, может измениться в будующих версиях терминала)
}

void ffc::initZMQ() {
	context = zmq_ctx_new();
	request = zmq_socket(context, ZMQ_XSUB);

	int counter = zmq_connect(request, SERVER_ADDR); assert(counter == 0);
	zmq_setsockopt(request, ZMQ_IDENTITY, "", 0);
	zmq_setsockopt(request, ZMQ_SUBSCRIBE, "", 0);

	validOrder = 0;
	updateOrder = 0;
	std::wcout << "Received: " << "connect = " << counter << "\r\n";
}

int ffc::zmqReceiveOrders(FfcOrder* master_orders) {
	zmq_msg_t reply;
	std::wcout << "zmq_msg_recv1: " << "\r\n";
	zmq_msg_init(&reply);
	std::wcout << "zmq_msg_recv2: " << "\r\n";
	zmq_msg_recv(&reply, request, 0);
	std::wcout << "zmq_msg_recv3: " << "\r\n";
	int totalMSG = zmq_msg_size(&reply);
	std::wcout << "zmq_msg_recv: " << totalMSG << "\r\n";
	memcpy(&validOrder, (int*)zmq_msg_data(&reply), sizeof(int));
	memcpy(&updateOrder, (int*)zmq_msg_data(&reply) + 1, sizeof(int));
	memcpy(master_orders, (int*)zmq_msg_data(&reply) + 2, totalMSG);

	std::wcout << "Received: " << "valid = " << validOrder << " master_orders= " << totalMSG << " update= " << updateOrder << "\r\n";
	zmq_msg_close(&reply);
	int count = totalMSG / sizeof(FfcOrder);
	return count;
}

void ffc::deInitZMQ() {
	zmq_close(request);
	zmq_ctx_destroy(context);
}


//cюда нужно получить инфу об уже имеющихся счетах и коктейлях
int ffc::initCocktails(long acc_number) {
	std::wcout << "account number: " << acc_number << "\r\n";
	// запрос на сервер, к какому коктейлю привязан аккаунт, пока забил вручную
	if (acc_number == 1732282 || acc_number == 751137852) {
	return TIM_COOK;
	}
	return false;
}


bool ffc::getCocktails(int provider, int name) {
	switch (name) {
	case TIM_COOK:
		if (provider == 1593142 || provider == 1627564 || provider == 1346753) return true;
		break;
	case BILL_GATES:
		if (provider == 500) return true;
		break;
	case V_PUTIN:
		if (provider == 1593142) return true;
		break;
	}
	return false;
}



bool ffc::showCocktails(int name) {

	return false;
}