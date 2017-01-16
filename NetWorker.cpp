#include "NetWorker.h"


void ffc::comSession() {
	std::string msg;
	std::string response;
	//newCom = false;

	mainPackage["accountBalance"] = 40;

	msg = mainPackage.dump().c_str();
	mainPackage.clear();
	response = send(msg);
	if (response != "") {
		json answer;
		answer = json::parse(response);
		if (!answer.empty()) {
			std::cout << "answer = " << answer << "\r\n";
			//AnswerHandler(answer);		//Обработка ответов сервера (lock)
		} else std::cout << "answer is empty!" << "\r\n";
	}
	//std::cout << "Error catch!" << "\r\n";

	std::cout << "ffc::ComSession() done" << "\r\n";
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
		/*form.set("accountNumber", std::to_string(accountNumber));
		form.set("advisorID", std::to_string(advisorID));
		form.set("accountCompany", accountCompany.c_str());
		form.set("serverKey", serverKey.c_str());
		form.set("codePage", std::to_string(GetACP()));*/
		form.set("data", msg.c_str());
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