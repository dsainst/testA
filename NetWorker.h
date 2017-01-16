#pragma once
#include "include/json.hpp"
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


#define bill_server "https://api.fairforex.org/index.php?r=site/test"



namespace ffc {

	using json = nlohmann::json;

	extern json			mainPackage;

	extern void comSession();
	std::string send(const std::string msg);


}