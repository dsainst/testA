#include "NetWorker.h"

long ffc::acc_number = 0;
bool ffc::isNetAllowed = true;
ffc::json staticInfo;
int			ffc::cocktail_id = 0;

void ffc::sendStaticInfo()
{
	std::cout << "Send static info!" << "\r\n";
	
	mainPackage["partnerID"] = PARTNER_ID;
	mainPackage["advisorVersion"] = ADVISOR_VER;
	mainPackage["accountInfo"] = staticInfo["acc"];
	newCom = true;
	isStaticSended = true;

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

void ffc::addOpenOrder(int _ticket, int _magic, std::string symbol, int _type, double _lots, double _openprice, double _tp, double _sl)
{
	nlohmann::json order;
	order["ticket"] = std::to_string(_ticket);
	order["magic"] = std::to_string(_magic);
	order["type"] = std::to_string(_type);
	order["lots"] = std::to_string(_lots);
	order["openprice"] = std::to_string(_openprice);
	order["tpprice"] = std::to_string(_tp);
	order["slprice"] = std::to_string(_sl);
	order["symbol"] = symbol.c_str();
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
	std::cout << "msg = " << msg << "\r\n";
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

	std::cout << "ffc::ComSession() done" << "\r\n";
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
	itr = answer.find("cocktails");
	if (itr != answer.end()) {
		std::cout << "cocktails1"  << "\r\n";
		json avr = itr.value();
		std::cout << "cocktails2" << "\r\n";
		std::cout << "cocktails3" << avr["providers"] << "\r\n";
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
		form.set("advisorID", std::to_string(ADVISOR_ID));
		form.set("accountCompany", accountCompany.c_str());
		form.set("serverKey", serverKey.c_str());
		form.set("codePage", std::to_string(GetACP()));
		if (!cocktail_id)
			form.set("needCocktail", std::to_string(4));
		form.set("data", msg.c_str());
		std::cout << "form number = " << accountNumber << "\r\n";
		std::cout << "form accountCompany = " << accountCompany << "\r\n";
		std::cout << "form currency = " << accountCurrency << "\r\n";
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