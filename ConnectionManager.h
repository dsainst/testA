#pragma once
#include <string>
#include "include/json.hpp"


#pragma pack(push,1)
#define bill_server "https://api.fairforex.org/index.php?r=site/test"
//work statuses
#define STATUS_NOT_INIT			-1
#define STATUS_OK				0
#define STATUS_DANGER			1
#define STATUS_SOFT_BREAK		2
#define STATUS_HARD_BREAK		3
#define STATUS_EMERGENCY_BREAK	4
#define MAIN_SESSION_PERIOD	60
#define DEFAULT_SERVER_KEY	"Promo2016"
#define DEFAULT_INIT_REASON	"not initialized"
#pragma pack(pop,1)

namespace ffc {
	void				comSession();

	static nlohmann::json	mainPackage;
	static bool			isStaticSended;
	static bool			isNetAllowed;		//�������� �� ������ ������ �����
	static bool			newCom;				//���� ����������, �� ����� ��� ����������
	static int			serverStatus;		//��������� ������ ���������� ��������
	static std::string	serverReason;		//��������� ������� ������� ����������
	static std::string	serverMessage;
	static std::string	clientErrors;
	static std::string	serverKey;			//���� �������
	static std::time_t	lastSession;		//����� ���������� ������
	static int			sessionPeriod;

	static int			accountNumber;		//����� ��������
	static std::string	accountCompany;

	static double		accountBalance;
	static double		accountEquity;
	static double		accountProfit;

	static __time64_t	firstTickTime;		//����� ����������� ������� ����

	static int			mainMagic;
	static __time64_t	expirationDate;
	static __int64		floatKey;
	static int			controlTicket;		//����� ������������ ������
	static int			numOrders;

	static int			advisorID;
	static std::string	partnerID;
	static int			advisorVersion;
	static std::string	advisorName;
	static std::string	regPath;
	static bool			isNetThreadActive;

	union double_int64 {
		double dval;
		__int64 ival;
	};


	static std::vector<int>				interestTickets;	//������ ���������� �������

	static void			sendStaticInfo();
	static void			setMyStatus();
	static int			updateAccountStep(double balance, double equity, double profit);
	static void			AnswerHandler(const nlohmann::json answer);
	static void			reset();
	static std::string	send(const std::string msg);

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
	static void			netWorker();
	static void			WriteKey(const std::string key);

}  //namespace ffc