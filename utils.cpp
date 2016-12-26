#include "string.h"
#include "utils.h"

#include "zmq.h"

#include <stdlib.h>
#include <thread>
#include <atomic>
#include <iostream>

//#define SERVER_ADDR "tcp://212.116.110.46:8083"
#define SERVER_ADDR "tcp://127.0.0.1:8083"

void* context;
void* request;
int ffc::validOrder;
int ffc::updateOrder;
FfcMsg ffc::msg = { 0 };
bool ffc::threadActive = false;
std::mutex ffc::mutex;

std::map<std::string, SymbolInfo> ffc::SymbolInfos;

int ffc::getMasterTicket(wchar_t* comment) {
	wchar_t* pwc;
	pwc = wcstok(comment, L"_");
	pwc = wcstok(NULL, L"_");
	if (pwc == NULL) return 0;
	//std::wcout << "getMagic " << comment << "/" << _wtoi(pwc) << "\r\n";
	return _wtoi(pwc);
}

int ffc::getMasterTicket2(int magic) {
	int res = MAGIC_EA_MASK & magic;
	if (res > 4095) {
		std::wcout << "���� ����! \r\n";
		return 0;
	}
	return res;
}

///�������� ������ �++ � MQLSting (�� ������� MQL ������ ������ ������ �� ����)
void ffc::writeMqlString(MqlString dest, wchar_t* source) {
	int len = min(wcslen(source), dest.size - 1);  //���������� ����� ������ (�������� ��������������� ������)
	wcscpy_s(dest.buffer, len + 1, source);  //�������� ������� ������������� ����
	*(((int*)dest.buffer) - 1) = len;  // ���������� ����� ������ (���, ����� ���������� � �������� ������� ���������)
}

std::string ffc::WC2MB(const wchar_t* line) {
	if (line == nullptr) {
		return std::string("null");
	}
	std::size_t len_ = WideCharToMultiByte(CP_UTF8, 0, line, -1, NULL, 0, NULL, NULL);
	std::string buff_(len_ + 1, 0);
	//std::wcstombs(&buff_[0], line, len_+1);
	WideCharToMultiByte(CP_UTF8, 0, line, -1, &buff_[0], len_, NULL, NULL);
	return buff_;
}

void ffc::initZMQ() {
	context = zmq_ctx_new();
	request = zmq_socket(context, ZMQ_SUB);

	int counter = zmq_connect(request, SERVER_ADDR); 
	//zmq_setsockopt(request, ZMQ_IDENTITY, "", 0);
	zmq_setsockopt(request, ZMQ_SUBSCRIBE, "", 0);

	validOrder = 0;
	updateOrder = 0;
	std::wcout << "Received: " << "connect = " << counter << "\r\n";
}

void ffc::zmqReceiveOrders() {
	threadActive = true;
	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, request, 0);
	int totalMSG = zmq_msg_size(&reply);
	mutex.lock();
	memcpy(&msg, zmq_msg_data(&reply), totalMSG);
	mutex.unlock();
	std::wcout << "Receiver size order - " << totalMSG << ". \r\n";
	std::wcout << "Received: valid = " << msg.validation << " count = " << msg.ordersCount << "\r\n";
	zmq_msg_close(&reply);
	threadActive = false;
}

void ffc::deInitZMQ() {
	zmq_close(request);
	zmq_ctx_destroy(context);
}


//c��� ����� �������� ���� �� ��� ��������� ������ � ���������
int ffc::initCocktails(long acc_number) {
	std::wcout << "account number: " << acc_number << "\r\n";
	// ������ �� ������, � ������ �������� �������� �������, ���� ����� �������
	//if (acc_number == 1732282 || acc_number == 751137852) { // 751137852 - demo
	return TIM_COOK;
	//}
	return false;
}


bool ffc::getCocktails(int provider, int name) {
	switch (name) {
	case TIM_COOK:
		if (provider == 1593142 || provider == 1627564 || provider == 1346753 || provider == 1555139) return true;
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