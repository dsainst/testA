#include <thread>
#include <stdio.h>

#include <Poco/URI.h>
#include <Poco/Path.h>
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"
#include "Poco/SharedPtr.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/OAuth10Credentials.h"

#include <Poco/Util/WinRegistryKey.h>

#include "ConnectionManager.h"



void ffc::sendStaticInfo()
{
	nlohmann::json j;

	j["acc"] = "2312";

	mainPackage["partnerID"] = "d3i92d3";
	mainPackage["advisorVersion"] = "1.2";
	mainPackage["accountInfo"] = j["acc"];
	newCom = true;
	isStaticSended = true;
}


void ffc::setMyStatus()
{
	if (serverStatus > STATUS_OK) {
		//stratptr->setStatus(PROVIDER_PERMISSION, serverStatus, serverReason.c_str());
	}
	else {
		//stratptr->setStatus(PROVIDER_PERMISSION, STATUS_SOFT_BREAK, "need server connection", expirationDate);
	}
}


int ffc::updateAccountStep(double balance, double equity, double profit)
{
	accountBalance = balance;
	accountEquity = equity;
	accountProfit = profit;
	if (firstTickTime > 0) {//Значит торговля уже началась
		return interestTickets.size() ? interestTickets[0] : 0;
	}
	else {  //Значит это инициализация и будем синхронизировать контрольный ордер
		firstTickTime = 1;
		if (controlTicket == 0 && isRegKeyExist("cTicket")) {
			controlTicket = getRegInt("cTicket");
			return controlTicket;
		}
		return 0;  //???Если контрольный ордер уже синхронизировали, незачем делать это снова
	}
}

void ffc::netWorker()
{

	serverKey = ffc::ReadKey();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//LOG_I("NetWorker started");
	while (isNetAllowed) {
		comSession();
		//LOG_INFO("ping...")
		for (int i = 0; !newCom && isNetAllowed && i < sessionPeriod * 10; i++) {  // Это для того, чтобы в случае выгрузки длл быстро выйти
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	//LOG_I("NetWorker ended");
	//fxc::NDC::deinit();

	//DEBUG_CATCH("NetWorker")
	//	LOG_DEINIT;
	isNetThreadActive = false;
}

void ffc::comSession() {
	std::string msg;
	std::string response;
	newCom = false;
	if (clientErrors != "") {
		mainPackage["clientErrors"] = clientErrors.c_str();
		clientErrors = "";
	}
	mainPackage["accountBalance"] = accountBalance;
	mainPackage["accountEquity"] = accountEquity;
	mainPackage["accountProfit"] = accountProfit;
	/*for (auto pair : openOrders) {
	nlohmann::json order;
	order["ticket"] = pair.first;// second->ticket;
	order["type"] = pair.second.type;
	order["symbol"] = pair.second.symbol.c_str();
	order["magic"] = pair.second.magic;
	order["lots"] = pair.second.lots;
	order["opentime"] = pair.second.opentime;
	order["openprice"] = pair.second.openprice;
	order["tpprice"] = pair.second.tpprice;
	order["slprice"] = pair.second.slprice;
	order["profit"] = pair.second.profit;
	mainPackage["openOrders"].push_back(order);
	}*/
	msg = mainPackage.dump().c_str();
	mainPackage.clear();
	//openOrders.clear();
	response = send(msg);
	if (response != "") {
		nlohmann::json answer;
		answer = nlohmann::json::parse(response);
		if (!answer.empty()) {
			//std::wcout << answer.get << "/r/n";
			//AnswerHandler(answer);		//Обработка ответов сервера (lock)
		}
		//track("ffc::ComSession() done");
	}
	else {
	}
}

void ffc::AnswerHandler(const nlohmann::json answer)
{
	bool statusUpdate = false;
	auto itr = answer.find("wantStatic");
	if (itr != answer.end()) {
		statusUpdate = true;
		sendStaticInfo();
	}
	itr = answer.find("serverStatus");
	if (itr != answer.end()) {
		statusUpdate = true;
		serverStatus = itr->get<int>();
	}
	itr = answer.find("serverReason");
	if (itr != answer.end()) {
		statusUpdate = true;
		serverReason = itr->get<std::string>().c_str();
	}
	//LOG_INFO("try find serverMessage")
	itr = answer.find("serverKey");
	if (itr != answer.end()) {
		serverKey = itr->get<std::string>().c_str();
		ffc::WriteKey(serverKey.c_str());
	}

	itr = answer.find("serverMessage");
	if (itr != answer.end()) {
		serverMessage = itr->get<std::string>().c_str();
	}
	//LOG_INFO("try find controlTicket\r\n")

	itr = answer.find("controlTicket");
	if (itr != answer.end()) {
		if (controlTicket == 0) {
			controlTicket = itr->get<int>();
		}
		else {
			controlTicket = min(controlTicket, itr->get<int>());
		}
		ffc::setRegKey("cTicket", controlTicket);
	}
	//LOG_INFO("try find interestTickets\r\n")

	itr = answer.find("interestTickets");
	if (itr != answer.end()) {
		//LOG_INFO("start update interestTickers...")
		interestTickets.clear();
		interestTickets = itr->get<std::vector<int>>();
	}
	itr = answer.find("checkSum");
	if (itr != answer.end()) {
		floatKey = itr->get<__int64>();
	}
	itr = answer.find("currentPattern");
	if (itr != answer.end()) {
		__int64 value = itr->get<__int64>();
		expirationDate = value ^ floatKey;
	}
	if (statusUpdate) {
		if (serverStatus == STATUS_OK) {
			if (_difftime64(_time64(NULL) + 3600, expirationDate) > 0) expirationDate = _time64(NULL) + 3600;
			//else LOG_D("lower");
		}
		else {
			expirationDate = 0;
		}
		//LOG_D("Status: %d, reason: %s", serverStatus, serverReason.c_str());
		char d1[32];
		_ctime64_s(d1, &expirationDate);

		ffc::setRegKey("currentPattern", expirationDate ^ floatKey);

		//LOG_INFO("Start change statuses - ")
		/*for (auto& entry : pool) {
		if (entry == nullptr) continue;
		if (entry->mqlTester || entry->mqlOptimization) {
		//entry->setStatus(PROVIDER_PERMISSION, STATUS_OK, "test is allowed");
		}
		else {
		if (serverStatus > STATUS_OK) {
		//entry->setStatus(PROVIDER_PERMISSION, serverStatus, serverReason.c_str());
		}
		else {
		//entry->setStatus(PROVIDER_PERMISSION, STATUS_SOFT_BREAK, "need server connection", expirationDate);
		}
		//LOG_T("AnswerHandler set statuses")
		}
		}*/
		//LOG_INFO("end change statuses\r\n")
	}
}

void ffc::reset() {
	//std::lock_guard<std::mutex> locker(mutex);
	newCom = false;
	lastSession = 0;		//Время последнего сеанса
	sessionPeriod = MAIN_SESSION_PERIOD;
	isStaticSended = false;

	accountNumber = 0;
	advisorID = 0;
	partnerID = "";
	advisorVersion = 0;

	serverStatus = STATUS_HARD_BREAK;
	serverReason = DEFAULT_INIT_REASON;
	serverMessage = "";
	clientErrors = "";

	accountBalance = 0.0;
	accountEquity = 0.0;
	accountProfit = 0.0;

	mainMagic = 0;	//???
	expirationDate = 0;	//???
	floatKey = 0;	//???
	controlTicket = 0;
	numOrders = 0;
	interestTickets.clear();
	//openOrders.clear();

	mainPackage.clear();
}

class SSLInitializer {
public:
	SSLInitializer() { Poco::Net::initializeSSL(); }

	~SSLInitializer() { Poco::Net::uninitializeSSL(); }
};

std::string ffc::send(const std::string msg) {
	//tracer;
	try {
		SSLInitializer sslInitializer;
		Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> ptrCert = new Poco::Net::AcceptCertificateHandler(false);
		Poco::Net::Context::Ptr ptrContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		Poco::Net::SSLManager::instance().initializeClient(0, ptrCert, ptrContext);
		Poco::URI uri(bill_server);
		Poco::Net::HTTPSClientSession client_session(uri.getHost(), uri.getPort());

		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";

		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPMessage::HTTP_1_1);

		Poco::Net::HTMLForm form;
		form.set("accountNumber", std::to_string(accountNumber));
		form.set("advisorID", std::to_string(advisorID));
		form.set("accountCompany", accountCompany.c_str());
		form.set("serverKey", serverKey.c_str());
		form.set("codePage", std::to_string(GetACP()));
		form.set("data", msg.c_str());
		form.prepareSubmit(req);

		form.write(client_session.sendRequest(req));
		//LOG_T("send message: %s", msg.c_str());

		Poco::Net::HTTPResponse res;

		std::istream &is = client_session.receiveResponse(res);
		std::stringstream ss;
		Poco::StreamCopier::copyStream(is, ss);
		//LOG_T("-> received: %s", ss.str().c_str());
		//LOG_T("send session done");
		return ss.str();
	}
	catch (Poco::Exception& e) {
		//LOG_E("=[ConnectionAdapter::send %s ====", e.message().c_str());
		std::wcout << e.message().c_str() << "/r/n";
		//CATCH("ffc::send()")
		return "";
	}
}

int	 ffc::netCount(int type)
{
	return 0;
}

#pragma region WinRegistry
void ffc::setRegKey(std::string key, int value)
{
	if (regPath.empty()) {
		//LOG_W("regPath is Empty, key: %s", key.c_str());
		return;
	}
	try {
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).setInt(key, value);
	}
	catch (Poco::Exception& e) {
		std::wcout << "setRegKey = " << e.message().c_str() << "/r/n";
	}
}
void ffc::setRegKey(std::string key, double value)
{
	try {
		ffc::double_int64 val;
		val.dval = value;
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).setInt64(key, val.ival);
	}
	catch (Poco::Exception& e) {
		std::wcout << "setRegKey = " << e.message().c_str() << "/r/n";
	}
}
void ffc::setRegKey(std::string key, __int64 value)
{
	try {
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).setInt64(key, value);
	}
	catch (Poco::Exception& e) {
		std::wcout << "setRegKey = " << e.message().c_str() << "/r/n";
	}
}
void ffc::setRegKey(std::string key, std::string value)
{
	try {
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).setString(key, value.c_str());
	}
	catch (Poco::Exception& e) {
		std::wcout << "setRegKey = " << e.message().c_str() << "/r/n";
	}
}
void ffc::setRegKey(std::string key, bool value)
{
	try {
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).setInt(key, (int)value);
	}
	catch (Poco::Exception& e) {
		std::wcout << "setRegKey = " << e.message().c_str() << "/r/n";
	}
}

int ffc::getRegInt(std::string key)
{
	int res;
	try {
		res = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).getInt(key);
	}
	catch (Poco::Exception& e) {
		std::wcout << "getRegInt = " << e.message().c_str() << "/r/n";
	}
	return res;
}
__int64 ffc::getRegInt64(std::string key)
{
	__int64 res;
	try {
		res = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).getInt64(key);
	}
	catch (Poco::Exception& e) {
		std::wcout << "getRegInt64 = " << e.message().c_str() << "/r/n";
	}
	return res;
}
double ffc::getRegDouble(std::string key)
{
	ffc::double_int64 val;
	try {
		val.ival = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).getInt64(key);
	}
	catch (Poco::Exception& e) {
		std::wcout << "getRegDouble = " << e.message().c_str() << "/r/n";
	}
	return val.dval;
}
std::string ffc::getRegString(std::string key)
{
	std::string res;
	try {
		res = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).getString(key);
	}
	catch (Poco::Exception& e) {
		std::wcout << "getRegString = " << e.message().c_str() << "/r/n";
	}
	return res;
}
bool ffc::getRegBool(std::string key)
{
	bool res = true;
	try {
		res = (bool)Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).getInt(key);
	}
	catch (Poco::Exception& e) {
		std::wcout << "getRegBool = " << e.message().c_str() << "/r/n";
	}
	return res;
}
bool ffc::isRegKeyExist(std::string key) {
	bool res;
	try {
		res = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).exists(key);
	}
	catch (Poco::Exception& e) {
		std::wcout << "isRegKeyExist = " << e.message().c_str() << "/r/n";
	}//DEBUG_CATCH("isRegKeyExist")
	return res;
}
//Необходима блокировка потока
std::string ffc::ReadKey()
{
	std::string res;
	try {
		if (Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, "Software\\FairForex").exists("serverKey")) {
			res = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, "Software\\FairForex").getString("serverKey").c_str();
		}
	}
	catch (Poco::Exception& e) {
		std::wcout << "ReadKey = " << e.message().c_str() << "/r/n";
	}
	return res;
}
//Необходима блокировка потока
void ffc::WriteKey(const std::string key)
{
	try {
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, "Software\\FairForex").setString("serverKey", key.c_str());
	}
	catch (Poco::Exception& e) {
		std::wcout << "WriteKey = " << e.message().c_str() << "/r/n";
	}
}
#pragma endregion