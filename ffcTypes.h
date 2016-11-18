#pragma once

#define SYMBOL_LENGTH	16
#define COMMENT_LENGTH	32
#define MAX_ORDER_COUNT	200
#define MAGIC_EA	0x70004000
#define MAGIC_EA_MASK	0x00000FFF


//Структура для передачи строк
#pragma pack(push,1)
struct MqlString
{
	int    size;     // 4 bytes размер буфера строки, не менять
	wchar_t* buffer;   // 4 bytes адрес буфера строки, не менять
					   //Длина строки хранится по адресу *(((int*)buffer) - 1)
	int    reserved; // 4 bytes
};
#pragma pack(pop,1)
#pragma pack(push,1)
struct FfcOrder
{
	int			ticket;
	int			magic;		//Provider
	wchar_t		symbol[SYMBOL_LENGTH];
	int			type;
	double		lots;		// Depo/Lot
	double		openprice;
	//__time64_t	opentime;
	double		tpprice;
	double		slprice;
	//double		closeprice;
	//__time64_t	closetime;
	__time64_t	expiration;
	//double		profit;
	//double		comission;
	//double		swap;
	//wchar_t		comment[COMMENT_LENGTH];
};
#pragma pack(pop,1)
#pragma pack(push,1)
struct FfcMsg
{
	int		validation;
	int		ordersCount;
	FfcOrder orders[MAX_ORDER_COUNT];
};
#pragma pack(pop,1)