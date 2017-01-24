#include "NetWorker.h"

long ffc::acc_number = 0;
bool ffc::isNetAllowed = true;
ffc::json staticInfo;
int ffc::cocktail_fill = 0;
bool ffc::newCom = false;

std::vector<int>				ffc::cocktails;

void ffc::sendStaticInfo()
{
	std::cout << "Send static info!" << "\r\n";
	
	mainPackage["partnerID"] = PARTNER_ID;
	mainPackage["advisorVersion"] = ADVISOR_VER;
	mainPackage["accountInfo"] = staticInfo["acc"];
	//isStaticSended = true;

	comSession();
}

void ffc::setAccount()
{
	if (accountNumber != acc_number) {
		if (accountNumber != 0) {
				reset();
		}
		serverKey = ffc::ReadKey();
		accountNumber = acc_number;
		accountCompany = DEFAULT_COMPANY_NAME;
		regPath = ffc::sformat("Software\\Anderson\\%d\\%s", accountNumber, advisorName.c_str());
	}
	if (expirationDate == 0 && isRegKeyExist("currentPattern")) {
		expirationDate = getRegInt64("currentPattern") ^ floatKey;
		if (_difftime64(expirationDate, _time64(NULL)) > EXPIRATION_LIMIT) expirationDate = 0;
	}
	//stratptr->setStatus(PROVIDER_PERMISSION, STATUS_SOFT_BREAK, "need server connection", expirationDate);
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


int ffc::updateAccountStep(TerminalS* TermInfo)
{
	staticInfo["acc"]["accountCurrency"] = WC2MB(TermInfo->currency).c_str();
	staticInfo["acc"]["accountCompany"] = WC2MB(TermInfo->companyName).c_str();
	staticInfo["acc"]["accountServer"] = WC2MB(TermInfo->server).c_str();
	staticInfo["acc"]["accountName"] = WC2MB(TermInfo->name).c_str();
	staticInfo["acc"]["accountNumber"] = std::to_string(accountNumber);
	staticInfo["acc"]["accountStopoutLevel"] = std::to_string(TermInfo->stoplevel);
	staticInfo["acc"]["accountStopoutMode"] = std::to_string(TermInfo->stopmode);
	staticInfo["acc"]["accountTradeMode"] = std::to_string(TermInfo->tradeMode);
	staticInfo["acc"]["accountLeverage"] = std::to_string(TermInfo->leverage);
	staticInfo["acc"]["accountLimitOrders"] = std::to_string(TermInfo->limit);
	staticInfo["acc"]["accountLimitOrders"] = std::to_string(TermInfo->limit);
	staticInfo["acc"]["accountFreeMarginMode"] = std::to_string(TermInfo->margin);

	accountBalance = TermInfo->balance;
	accountEquity = TermInfo->equity;
	accountProfit = TermInfo->profit;
	accountCurrency = WC2MB(TermInfo->currency);
	accountCompany = WC2MB(TermInfo->companyName);

	return 0;
}

int ffc::updateOrderClosed(int _ticket, int _type, int _magic, std::string _symbol, double _lots, __time64_t _opentime, double _openprice, double _tp, double _sl, __time64_t _closetime, double _closeprice, double _profit)
{
	int next = 0;
	auto itr = interestTickets.begin();
	while (itr != interestTickets.end()) {
		if (*itr == _ticket) {
			if ((_type == 0 || _type == 1) && _closetime > 0) {  //Если ордер не найден на стороне mql его маркируем _type = -1
				json order;
				order["ticket"] = _ticket;
				order["symbol"] = _symbol.c_str();
				order["type"] = _type;
				order["magic"] = _magic;
				order["lots"] = _lots;
				order["opentime"] = _opentime;
				order["openprice"] = _openprice;
				order["tpprice"] = _tp;
				order["slprice"] = _sl;
				order["closetime"] = _closetime;
				order["closeprice"] = _closeprice;
				order["profit"] = _profit;
				mainPackage["closedOrders"].push_back(order);
			}
			itr = interestTickets.erase(itr);
			if (itr != interestTickets.end()) {
				next = *itr;
			}
			else if (mainPackage.find("closedOrders") != mainPackage.end()) { //если есть что отослать, отсылаем
			newCom = true;
			}
			break;
		}
		else { itr++; }
	}
	return next;
}

void ffc::addOpenOrder(int _ticket, int _magic, std::string _symbol, int _type, double _lots, double _openprice, __time64_t _opentime, double _tp, double _sl, int _provider)
{
	nlohmann::json order;
	order["ticket"] = _ticket;
	order["magic"] = _magic;
	order["type"] = _type;
	order["lots"] = _lots;
	order["openprice"] = _openprice;
	order["opentime"] = _opentime;
	order["profit"] = 0;
	order["tpprice"] = _tp;
	order["slprice"] = _sl;
	order["symbol"] = _symbol.c_str();
	order["provider"] = _provider;
	mainPackage["openOrders"].push_back(order);
}

void ffc::netWorker()
{
	serverKey = ffc::ReadKey();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	
	while (isNetAllowed) {
		std::this_thread::sleep_for(std::chrono::milliseconds(60000));
	}
}

void ffc::comSession() {
	std::string msg;
	std::string response;
	newCom = false;

	serverKey = ffc::ReadKey();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	mainPackage["accountBalance"] = accountBalance;
	mainPackage["accountEquity"] = accountEquity;
	mainPackage["accountProfit"] = accountProfit;


	msg = mainPackage.dump().c_str();
	//std::cout << "msg = " << msg << "\r\n";
	mainPackage.clear();
	response = send(msg);
	if (response != "") {
		json answer;
		answer = json::parse(response);
		if (!answer.empty()) {
			std::cout << "answer = " << answer << "\r\n";
			AnswerHandler(answer);		//Обработка ответов сервера (lock)
		} else std::cout << "answer is empty!" << "\r\n";
	}

	//std::cout << "ffc::ComSession() done" << "\r\n";
}

void ffc::AnswerHandler(const json answer)
{
	bool statusUpdate = false;
	auto itr = answer.find("needStatic");
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
		std::cout << "start update interestTickers...\r\n";
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

	itr = answer.find("cocktails");
	if (itr != answer.end()) {
		json avr = itr.value();
		auto itr2 = avr.find("providers");
		if (itr2 != avr.end()) {
			cocktails.clear();
			cocktails = itr2->get<std::vector<int>>();
			cocktail_fill = 1;
		} else std::cout << "providers is not found! " << "\r\n";
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
	//isStaticSended = false;

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

	expirationDate = 0;	//???
	floatKey = 0;	//???
	controlTicket = 0;
	numOrders = 0;
	staticInfo.clear();
	interestTickets.clear();

	mainPackage.clear();
}

class SSLInitializer {
public:
	SSLInitializer() { Poco::Net::initializeSSL(); }

	~SSLInitializer() { Poco::Net::uninitializeSSL(); }
};

std::string ffc::send(const std::string _msg) {
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
		form.set("advisorID", std::to_string(ADVISOR_ID));
		form.set("accountCompany", accountCompany.c_str());
		form.set("serverKey", serverKey.c_str());
		form.set("codePage", std::to_string(GetACP()));
		if (!cocktail_fill)
			form.set("needCocktail", std::to_string(COCKTAIL_ID));
		form.set("data", _msg.c_str());
		//std::cout << "account number = " << accountNumber << "\r\n";
		//std::cout << "account Company = " << accountCompany << "\r\n";
		form.prepareSubmit(req);

		form.write(client_session.sendRequest(req));

		Poco::Net::HTTPResponse res;

		std::istream &is = client_session.receiveResponse(res);
		std::stringstream ss;
		Poco::StreamCopier::copyStream(is, ss);
		return ss.str();
	}
	catch (Poco::Exception& e) {
		std::wcout << e.message().c_str() << "/r/n";
		return "";
	}
}

int	 ffc::netCount(int type)
{
	return 0;
}

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
		if (Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, regPath).getInt(key) != 1) res = false;
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
		if (Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, "Software\\Anderson").exists("serverKey")) {
			res = Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, "Software\\Anderson").getString("serverKey").c_str();
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
		Poco::Util::WinRegistryKey(HKEY_CURRENT_USER, "Software\\Anderson").setString("serverKey", key.c_str());
	}
	catch (Poco::Exception& e) {
		std::wcout << "WriteKey = " << e.message().c_str() << "/r/n";
	}
}