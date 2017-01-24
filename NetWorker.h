#pragma once
#include "include/json.hpp"
#include "ffcTypes.h"
#include "utils.h"
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

#pragma pack(push,1)
#define bill_server "https://api.fairforex.org/" //index.php?r=site/test
//work statuses
#define STATUS_NOT_INIT			-1
#define STATUS_OK				0
#define STATUS_DANGER			1
#define STATUS_SOFT_BREAK		2
#define STATUS_HARD_BREAK		3
#define STATUS_EMERGENCY_BREAK	4
#define MAIN_SESSION_PERIOD		60
#define DEFAULT_INIT_REASON		"not initialized"
#define DEFAULT_COMPANY_NAME	"FFAnderson"

#define SYMBOL_MAX_LENGTH		255

#define BASE_KEY64		0x70A12283F4B536C7
#define EXPIRATION_LIMIT 3*24*60*60

#define TIME_CONNECT_BILLING		9		// время в секундах - через которое идет коннект с биллингом
#define COCKTAIL_ID					8		// id коктейля


#define PROJECT_URL	   "fairforex.org"
#define EXPERT_NAME    "FFAnderson"
#define MAGIC_EA		0x70004000
#define PARTNER_ID		""//YrDm6Gp-SKLF5fQDUDoD"

#define ADVISOR_VER		1016
#define ADVISOR_ID		4

struct TerminalS
{
	double			balance;
	double			equity;
	double			profit; 
	int				tradeMode;
	double			margin;
	int				leverage;
	int				limit;
	int				stoplevel;
	int				stopmode;
	wchar_t			currency[SYMBOL_LENGTH];
	wchar_t			companyName[SYMBOL_MAX_LENGTH];
	wchar_t			name[SYMBOL_MAX_LENGTH];
	wchar_t			server[SYMBOL_MAX_LENGTH];
};
#pragma pack(pop,1)





namespace ffc {

	using json = nlohmann::json;

	extern int			cocktail_fill;

	extern json			mainPackage;
	extern long			acc_number;
	//static bool			isStaticSended;
	extern bool			newCom;				//Если установлен, то сеанс вне расписания
	static int			serverStatus	= STATUS_NOT_INIT;		//Серверный статус разрешения торговли
	static std::string	serverReason;		//Серверная причина статуса разрешения
	static std::string	serverMessage;
	static std::string	clientErrors;
	static std::string	serverKey;			//Ключ доступа
	static std::time_t	lastSession;		//Время последнего сеанса
	static int			sessionPeriod;

	static int			accountNumber;		//Номер аккаунта
	static std::string	accountCompany;
	static std::string	accountCurrency;         // наименование валюты текущего счета

	static double		accountBalance;
	static double		accountEquity;
	static double		accountProfit;

	static __time64_t	expirationDate;
	static __int64		floatKey;
	static int			controlTicket;		//Тикет контрольного ордера
	static int			numOrders;

	static int			advisorID;
	static std::string	partnerID;
	static int			advisorVersion;
	static std::string	advisorName;
	static std::string	regPath;
	extern bool			isNetAllowed;		//Разрешен ли запуск модуля связи

	union double_int64 {
		double dval;
		__int64 ival;
	};

	static std::vector<int>				interestTickets;	//Тикеты интересных ордеров
	extern std::vector<int>				cocktails;			//Провайдеры, в зависимости от коктейля

	extern void			comSession();
	static void			sendStaticInfo();
	static void			setMyStatus();
	int					updateAccountStep(TerminalS* TermInfo);

	int					updateOrderClosed(int _ticket, int _type, int _magic, std::string _symbol, double _lots, __time64_t _opentime, double _openprice, double _tp, double _sl, __time64_t _closetime, double _closeprice, double _profit);

	extern void			addOpenOrder(int _ticket, int _magic, std::string symbol, int _type, double _lots, double _openprice, __time64_t _opentime, double _tp, double _sl, int _provider);

	static void			AnswerHandler(const nlohmann::json answer);
	static void			reset();
	std::string			send(const std::string msg);

	static int			netCount(int type);
	static void			setRegKey(std::string key, int			value);
	static void			setRegKey(std::string key, __int64		value);
	static void			setRegKey(std::string key, double		value);
	static void			setRegKey(std::string key, std::string	value);
	static void			setRegKey(std::string key, bool			value);
	static int			getRegInt(std::string key);
	static __int64		getRegInt64(std::string key);
	static double		getRegDouble(std::string key);
	static std::string	getRegString(std::string key);
	static bool			getRegBool(std::string key);
	static bool			isRegKeyExist(std::string key);
	static std::string	ReadKey();
	extern void			netWorker();
	void				setAccount();
	static void			WriteKey(const std::string key);


}