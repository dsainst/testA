#include "string.h"
#include "utils.h"

#include "zmq.h"
#include <fstream>

#include <stdlib.h>
#include <thread>
#include <atomic>
#include <iostream>

#define SERVER_ADDR "tcp://212.116.110.46:8083"
//#define SERVER_ADDR "tcp://127.0.0.1:8083"

void* context;
void* request;
int ffc::validOrder;
int ffc::updateOrder;
FfcMsg ffc::msgServer;
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
		std::wcout << "Вать машу! \r\n";
		return 0;
	}
	return res;
}

std::string ffc::sformat(const char *fmt, ...)
{
	char msg_buf[255];
	try {
		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(msg_buf, 254, fmt, ap);
		va_end(ap);
	}
	catch (...) {
		return std::string("sformat error");
	}
	return std::string(msg_buf);
}

// Копируем строку с++ в MQLSting (на стороне MQL больше ничего делать не надо)
void ffc::writeMqlString(MqlString dest, wchar_t* source) {
	int len = min(wcslen(source), dest.size - 1);  //Определяем длину строки (небольше распределенного буфера)
	wcscpy_s(dest.buffer, len + 1, source);  //Копируем включая терминирующий ноль
	*(((int*)dest.buffer) - 1) = len;  // Записываем длину строки (хак, может измениться в будующих версиях терминала)
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
	if (!context) return;
	request = zmq_socket(context, ZMQ_SUB);
	if (!request) return;

	int counter = zmq_connect(request, SERVER_ADDR); 
	//zmq_setsockopt(request, ZMQ_IDENTITY, "", 0);
	zmq_setsockopt(request, ZMQ_SUBSCRIBE, "", 0);

	validOrder = 0;
	updateOrder = 0;
	std::wcout << "Received: " << "connect = " << counter << "\r\n";
}

bool ffc::zmqReceiveOrders() {
	threadActive = true;
	zmq_msg_t reply;
	if (zmq_msg_init(&reply) == -1) {
		deInitZMQ();
		initZMQ();
		std::cout << "zmq_msg_init wrong \r\n";
		return false;
	};
	int ret = zmq_msg_recv(&reply, request, ZMQ_DONTWAIT);
	if (ret <= 0) {
		if (errno != EAGAIN) {
			std::cout << "zmq error: " << errno << "\r\n";
			deInitZMQ();
			initZMQ();
		}
		return false;
	}
	int totalMSG = zmq_msg_size(&reply);
	std::lock_guard<std::mutex> locker(mutex);
	msgServer = {};
	memcpy(&msgServer, zmq_msg_data(&reply), totalMSG);
	std::wcout << "Receiver size order - " << totalMSG << ". \r\n";
	std::wcout << "Received: valid = " << msgServer.validation << " count = " << msgServer.ordersCount << "\r\n";
	zmq_msg_close(&reply);
	threadActive = false;
	return true;
}

void ffc::deInitZMQ() {
	zmq_close(request);
	zmq_ctx_destroy(context);
	std::cout << "deInitZMQ \r\n";
}

void ffc::LogFile(std::string  line) {
	std::ofstream file;
	//can't enable exception now because of gcc bug that raises ios_base::failure with useless message
	//file.exceptions(file.exceptions() | std::ios::failbit);
	file.open("M:\\dev\\FF_Anderson\\Release\\log_Anderson.txt", std::ios::out | std::ios::app); //C:\\Users\\Admin\\AppData\\Roaming\\MetaQuotes\\Terminal\\C6D03BEE984A8FF7763AA4060BA5C4AC\\MQL4\\Experts
	if (file.fail())
		throw std::ios_base::failure(std::strerror(errno));

	//make sure write fails with exception if something is wrong
	file.exceptions(file.exceptions() | std::ios::failbit | std::ifstream::badbit);

	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	file << asctime(timeinfo) << line.c_str() << "\r\n---------------------------------------\r\n";

}