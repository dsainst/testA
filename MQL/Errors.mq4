void CriticalError(string msg)
{
	if (!showinfo) return;
}

//Обработать ошибку
void ShowError(string message, int err = -1, bool critical = false) {
	if (mqlOptimization)
		return;

	if (critical || ShowDebug)
	{
		Print(message, Error(err));
		FileWrite(herror, "-<[ ", TimeCurrent(), " ]>-----------------------------------------------------------");
		FileWrite(herror, message);
		FileWrite(herror, "Ask/Bid: ", Ask, "/", Bid);
	}

	timeout = fmax(timeout, 1000);  //При любой ошибке минимум секунду задержки
}

void ShowActionError(OrderAction& action, string message, int err = -1, bool critical = false)
{
	if (mqlOptimization)
		return;

	if (critical || ShowDebug)
	{
		Print(message, Error(err));
		FileWrite(herror, "-<[ ", TimeCurrent(), " ]>-----------------------------------------------------------");
		FileWrite(herror, StringConcatenate("Error ", err, ": ", message));
		FileWrite(herror, "Ask/Bid: ", Ask, "/", Bid);
		FileWrite(herror, 
			"Next type: ",    action.type, 
			", ticket: ",     action.ticket, 
			", lots: ",       action.lots, 
			", open price: ", action.openprice,
			", tp price: ",   action.tpprice, 
			", sl price: ",   action.slprice
		);
	}

	timeout = fmax(timeout, 1000);  //При любой ошибке минимум секунду задержки
}
void ShowActionError2(OrderAction& oldaction, OrderAction& action, string message, int err = -1, bool critical = false)
{
	if (mqlOptimization)
		return;

	if (critical || ShowDebug)
	{
		Print(message, Error(err));
		FileWrite(herror, "-<[ ", TimeCurrent(), " ]>-----------------------------------------------------------");
		FileWrite(herror, StringConcatenate("Error ", err, ": ", message));
		FileWrite(herror, "Ask/Bid: ", Ask, "/", Bid);
		FileWrite(herror, 
			"Next type: ",    oldaction.type, 
			", ticket: ",     oldaction.ticket, 
			", lots: ",       oldaction.lots, 
			", open price: ", oldaction.openprice,
			", tp price: ",   oldaction.tpprice, 
			", sl price: ",   oldaction.slprice,
			" =>"
		);
		FileWrite(herror, 
			"Next type: ",    action.type, 
			", ticket: ",     action.ticket, 
			", lots: ",       action.lots, 
			", open price: ", action.openprice,
			", tp price: ",   action.tpprice, 
			", sl price: ",   action.slprice
		);
	}

	timeout = fmax(timeout, 1000);  //При любой ошибке минимум секунду задержки
}

// Обработка ошибок по коду в соответствии с рекомендациями
void Err(int id)
{
	switch (id) {
		case 4 :
		case 132 :
			timeout = fmax(timeout, 60000);
			break;
		case 6 :
		case 129 :
		case 130 :
		case 136 :
			timeout = fmax(timeout, 5000);
			break;
		case 128 :
		case 142 :
		case 143 :
			timeout = fmax(timeout, 60000);
			break;
		case 145 :
			timeout = fmax(timeout, 15000);
			break;
		case 146 :
			timeout = fmax(timeout, 10000);
			break;
	}
}

// Возвращает описание ошибки на русском
string Error(int id)
{
	string res = "";

	switch (id) {
		case -1:   res=""; break;
		case 0:    res=" Нет ошибок. "; break;
		case 1:    res=" Нет ошибки, но результат неизвестен. "; break;
		case 2:    res=" Общая ошибка. "; break;
		case 3:    res=" Неправильные параметры. "; break;
		case 4:    res=" Торговый сервер занят. "; break;
		case 5:    res=" Старая версия клиентского терминала. "; break;
		case 6:    res=" Нет связи с торговым сервером. "; break;
		case 7:    res=" Недостаточно прав. "; break;
		case 8:    res=" Слишком частые запросы. "; break;
		case 9:    res=" Недопустимая операция нарушающая функционирование сервера. "; break;
		case 64:   res=" Счет заблокирован. "; break;
		case 65:   res=" Неправильный номер счета. "; break;
		case 128:  res=" Истек срок ожидания совершения сделки. "; break;
		case 129:  res=" Неправильная цена. "; break;
		case 130:  res=" Неправильные стопы. "; break;
		case 131:  res=" Неправильный объем. "; break;
		case 132:  res=" Рынок закрыт. "; break;
		case 133:  res=" Торговля запрещена. "; break;
		case 134:  res=" Недостаточно денег для совершения операции. "; break;
		case 135:  res=" Цена изменилась. "; break;
		case 136:  res=" Нет цен. "; break;
		case 137:  res=" Брокер занят. "; break;
		case 138:  res=" Новые цены. "; break;
		case 139:  res=" Ордер заблокирован и уже обрабатывается. "; break;
		case 140:  res=" Разрешена только покупка. "; break;
		case 141:  res=" Слишком много запросов. "; break;
		case 145:  res=" Модификация запрещена, так как ордер слишком близок к рынку. "; break;
		case 146:  res=" Подсистема торговли занята. "; break;
		case 147:  res=" Использование даты истечения ордера запрещено брокером. "; break;
		case 148:  res=" Количество открытых и отложенных ордеров достигло предела, установленного брокером. "; break;
		case 4000: res=" Нет ошибки. "; break;
		case 4001: res=" Неправильный указатель функции. "; break;
		case 4002: res=" Индекс массива - вне диапазона. "; break;
		case 4003: res=" Нет памяти для стека функций. ";break;
		case 4004: res=" Переполнение стека после рекурсивного вызова. "; break;
		case 4005: res=" На стеке нет памяти для передачи параметров. "; break;
		case 4006: res=" Нет памяти для строкового параметра. "; break;
		case 4007: res=" Нет памяти для временной строки. "; break;
		case 4008: res=" Неинициализированная строка. "; break;
		case 4009: res=" Неинициализированная строка в массиве. ";break;
		case 4010: res=" Нет памяти для строкового массива. "; break;
		case 4011: res=" Слишком длинная строка. "; break;
		case 4012: res=" Остаток от деления на ноль. "; break;
		case 4013: res=" Деление на ноль. "; break;
		case 4014: res=" Неизвестная команда. "; break;
		case 4015: res=" Неправильный переход. "; break;
		case 4016: res=" Неинициализированный массив. "; break;
		case 4017: res=" Вызовы DLL не разрешены. "; break;
		case 4018: res=" Невозможно загрузить библиотеку. "; break;
		case 4019: res=" Невозможно вызвать функцию. "; break;
		case 4020: res=" Вызовы внешних библиотечных функций не разрешены. "; break;
		case 4021: res=" Недостаточно памяти для строки, возвращаемой из функции. "; break;
		case 4022: res=" Система занята. "; break;
		case 4050: res=" Неправильное количество параметров функции. "; break;
		case 4051: res=" Недопустимое значение параметра функции. "; break;
		case 4052: res=" Внутренняя ошибка строковой функции. "; break;
		case 4053: res=" Ошибка массива. "; break;
		case 4054: res=" Неправильное использование массива-таймсерии. "; break;
		case 4055: res=" Ошибка пользовательского индикатора. "; break;
		case 4056: res=" Массивы несовместимы. "; break;
		case 4057: res=" Ошибка обработки глобальныех переменных. "; break;
		case 4058: res=" Глобальная переменная не обнаружена. "; break;
		case 4059: res=" Функция не разрешена в тестовом режиме. "; break;
		case 4060: res=" Функция не разрешена. "; break;
		case 4061: res=" Ошибка отправки почты. "; break;
		case 4062: res=" Ожидается параметр типа string. "; break;
		case 4063: res=" Ожидается параметр типа integer. "; break;
		case 4064: res=" Ожидается параметр типа double. "; break;
		case 4065: res=" В качестве параметра ожидается массив. "; break;
		case 4066: res=" Запрошенные исторические данные в состоянии обновления. "; break;
		case 4067: res=" Ошибка при выполнении торговой операции. "; break;
		case 4099: res=" Конец файла. "; break;
		case 4100: res=" Ошибка при работе с файлом. "; break;
		case 4101: res=" Неправильное имя файла. "; break;
		case 4102: res=" Слишком много открытых файлов. "; break;
		case 4103: res=" Невозможно открыть файл. "; break;
		case 4104: res=" Несовместимый режим доступа к файлу. "; break;
		case 4105: res=" Ни один ордер не выбран. "; break;
		case 4106: res=" Неизвестный символ. "; break;
		case 4107: res=" Неправильный параметр цены для торговой функции. "; break;
		case 4108: res=" Неверный номер тикета. "; break;
		case 4109: res=" Торговля не разрешена. Необходимо включить опцию Разрешить советнику торговать в свойствах эксперта. "; break;
		case 4110: res=" Длинные позиции не разрешены. Необходимо проверить свойства эксперта. "; break;
		case 4111: res=" Короткие позиции не разрешены. Необходимо проверить свойства эксперта. "; break;
		case 4200: res=" Объект уже существует. "; break;
		case 4201: res=" Запрошено неизвестное свойство объекта. "; break;
		case 4202: res=" Объект не существует. "; break;
		case 4203: res=" Неизвестный тип объекта. "; break;
		case 4204: res=" Нет имени объекта. "; break;
		case 4205: res=" Ошибка координат объекта. "; break;
		case 4206: res=" Не найдено указанное подокно. "; break;
		case 200 : res=" Ордер не найден. "; break;
		case 201 : res=" Стоплосс или тейкпрофит слишком близок к цене открытия. "; break;
		case 202 : res=" Цена открытия отложенного ордера не верна. "; break;
		default :  res=" Неизвестная ошибка. ";
	}

	return (res);
}