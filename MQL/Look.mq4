class Look
{
public:
	int symbol_height, symbol_width;
	int window_height, window_width;
	int header_height, header_width;
	int info_height, info_width;
	int pos_x, pos_y;
	int pad;	//Отступ
	int header_rows;
	int cur_row, rows, old_rows;
	
	void Look(int font_size, color text_color, int x, int y, string font_name = "Lucida Console")
	{
		_fontSize = font_size;
		_fontName = font_name;
		_textColor = text_color;
		pos_x = x;
		pos_y = y;
		kdpi = ffc_getDpi() / 72.0;
		TextSetFont(font_name, font_size);
		TextGetSize("WpWWWWWWWW", symbol_width, symbol_height);
		//Коррекция на разрешение
		symbol_width = (int)round(symbol_width * kdpi /10);
		symbol_height = (int)round(symbol_height * kdpi);
		cur_row = 0;
		rows = 0;
		pad = 5;
		header_rows = 0;

		_logoW = 0;
		_logoH = 0;
	}

	void Init()
	{
		MakeRect(pos_x, pos_y, 0, 0, C'0,80,0', C'0,150,0', "windowbox");
		MakeRect(pos_x+2, pos_y+ header_height, 0, 0, C'0,50,0', C'0,150,0', "infobox");
	}
	void SetHeader(int idx, string text) {
		header_rows = fmax(header_rows, idx + 1);
		string name = "header" + IntegerToString(idx);
		header_height = fmax(header_height, header_rows*symbol_height + pad * 2);
		header_height = fmax(header_height, _logoH + 2);
		header_width = fmax(header_width, _logoW + pad * 2 + StringLen(text)*symbol_width);
		
		if (ObjectFind(0, name) == -1) {
			MakeLabel(name, pos_x + pad + _logoW, pos_y + pad + idx*symbol_height,
				text, _fontSize, _textColor, _fontName);
		}
		else {
			ObjectSetString(0, name, OBJPROP_TEXT, text);
		}
	}


	void Set(string label, int row = 0, string url = "") {
		if (row == 0) {
			cur_row++;
		}
		else {
			cur_row = row;
			//Print("indexed row! ", label);
		}
		rows = fmax(rows, cur_row);
		info_width = fmax(info_width, StringLen(label)*symbol_width + pad * 2);
		string name = "row" + IntegerToString(cur_row-1);
		if (ObjectFind(0, name) == -1) {
			MakeLabel(name, pos_x + pad, pos_y + header_height + symbol_height*(cur_row-1) + pad,
				label, _fontSize, _textColor, _fontName);
			//Print("row: ", cur_row, "  pos-y: ", y + header_dy + sh*(cur_row-1) + pad);
		}
		else {
			ObjectSetString(0, name, OBJPROP_TEXT, label);
		}
	}

	void DrawHistory()
	{
		int ord_total     = OrdersHistoryTotal();
		datetime cur_date = TimeCurrent();
		int seconds       = 86400 * DrawHistoryDays;

		datetime open_date;
		datetime close_date;
		double open_price, close_price;
		int ticket;
		int _type;

		for (int i = ord_total; i >= 0; i--)
		{
			if (!OrderSelect(i, SELECT_BY_POS, MODE_HISTORY))
				continue;

			close_date = OrderCloseTime();
			_type      = OrderType();

			if (cur_date - close_date <= seconds && _type <= OP_SELL && OrderSymbol() == symbolName)
			{
				open_date   = OrderOpenTime();
				open_price  = OrderOpenPrice();
				close_price = OrderClosePrice();
				ticket      = OrderTicket();
				DrawOrder(ticket, _type, open_date, open_price, close_date, close_price);
			}
		}
	}
	
	void DrawOrder(int _ticket, int _type, datetime _open_date, double _open_price, datetime _close_date, double _close_price)
	{
		if (_close_price == 0)
			return;

		string open_name  = DoubleToStr(_ticket, 0) + " o";
		string close_name = DoubleToStr(_ticket, 0) + " c";
		string line_name  = DoubleToStr(_ticket, 0) + " l";

		// создаЄм открывающую стрелку
		ObjectCreate(open_name, OBJ_ARROW, 0, _open_date, _open_price); // открывающа¤ стрелка Ѕай-ордера
		ObjectSet(open_name, OBJPROP_ARROWCODE, 1);                     // код стрелки 232 
		ObjectSet(open_name, OBJPROP_COLOR, colors[_type]);             // цвет стрелки
		//ObjectSetInteger(0, open_name, OBJPROP_BACK, true);
		
		// создаЄм закрывающую стрелку
		ObjectCreate(close_name, OBJ_ARROW, 0, _close_date, _close_price); // закрывающа¤ стрелка Ѕай-ордера
		ObjectSet(close_name, OBJPROP_ARROWCODE, 3);                       // код стрелки 231
		ObjectSet(close_name, OBJPROP_COLOR, colors[_type]);               // цвет стрелки
		//ObjectSetInteger(0, close_name, OBJPROP_BACK, true);
		
		// создаЄм линии
		ObjectCreate(line_name, OBJ_TREND, 0, _open_date, _open_price, _close_date, _close_price);
		ObjectSet(line_name, OBJPROP_RAY, false);           // запрещаем рисовать луч
		ObjectSet(line_name, OBJPROP_WIDTH, 0);             // устанавливаем толщину линии
		ObjectSet(line_name, OBJPROP_STYLE, 0);             // устанавливаем тип линии (отрезки)
		ObjectSet(line_name, OBJPROP_STYLE, STYLE_DOT);     // устанавливаем тип штриховки (пунктирна¤ лини¤)
		ObjectSet(line_name, OBJPROP_COLOR, colors[_type]); // устанавливаем цвет(синий/красный)
		ObjectSetInteger(0, line_name, OBJPROP_BACK, true);
	}
	void DrawLine(string _name, datetime _open_date, double _open_price, datetime _close_date, double _close_price, color _color)
	{
		string line_name = IntegerToString(_open_date) + "-" +_name;
		ObjectCreate(line_name, OBJ_TREND, 0, _open_date, _open_price, _close_date, _close_price);
		ObjectSet(line_name, OBJPROP_RAY, false);           // запрещаем рисовать луч
		ObjectSet(line_name, OBJPROP_WIDTH, 0);             // устанавливаем толщину линии
		ObjectSet(line_name, OBJPROP_STYLE, STYLE_SOLID);     // устанавливаем тип штриховки (сплошная линия)
		ObjectSet(line_name, OBJPROP_COLOR, _color); // устанавливаем цвет(синий/красный)
		ObjectSetInteger(0, line_name, OBJPROP_BACK, true);
		
	}
	void DelOldDraws()
	{
		string _name;
		int _type;

		for (int i = 0; i<ObjectsTotal(); i++)
		{
			_name = ObjectName(i);
			_type = ObjectType(_name);

			if (_type != OBJ_ARROW && _type != OBJ_TREND)
				continue;

			if (ObjectGetInteger(0, _name, OBJPROP_TIME) < (TimeCurrent() - 86400 * DrawHistoryDays))
				ObjectDelete(_name);
		}
	}

	void ResizeBox() {
		info_height = rows*symbol_height + pad*2;
		window_height = header_height + info_height + 2;
		window_width = fmax(header_width, info_width + 4);
		ObjectSetInteger(0, "infobox", OBJPROP_XSIZE, window_width - 4);
		ObjectSetInteger(0, "infobox", OBJPROP_YSIZE, info_height);
		ObjectSetInteger(0, "infobox", OBJPROP_YDISTANCE, pos_y + header_height);
		ObjectSetInteger(0, "windowbox", OBJPROP_XSIZE, window_width);
		ObjectSetInteger(0, "windowbox", OBJPROP_YSIZE, window_height);
		//Если количество строк уменьшилось
		for (int i = rows; i < old_rows; i++) {
			ObjectDelete(0, "row" + IntegerToString(i));
		}
		old_rows = rows;
		rows = 0;
		cur_row = 0;
		info_width = 0;
		ChartRedraw();
	}

private:

	int    _letterW, _letterH;
	int    _cols, _rows;
	int    _rowsLen[];   // длина рядов, как объекты
	int    _rowsCol[];   // длина рядов, как строки
	string _names[][32]; // имена объектов в рядах

	int    _fontSize;
	string _fontName;
	color  _textColor;

	int _padX, _padY;
	int _logoW, _logoH;
	int _headerH, _headerW;
	int _headerFontSize;

	void MakeRect(int px, int py, int w, int h, color fill, color border, string name = "box")
	{
		ObjectCreate(name, OBJ_RECTANGLE_LABEL, 0, 0, 0);
		ObjectSetInteger(0, name, OBJPROP_XDISTANCE, px);
		ObjectSetInteger(0, name, OBJPROP_YDISTANCE, py);
		ObjectSetInteger(0, name, OBJPROP_XSIZE, w);
		ObjectSetInteger(0, name, OBJPROP_YSIZE, h);
		ObjectSetInteger(0, name, OBJPROP_BGCOLOR, fill);
		ObjectSetInteger(0, name, OBJPROP_BORDER_TYPE, BORDER_FLAT);
		ObjectSetInteger(0, name, OBJPROP_CORNER, CORNER_LEFT_UPPER);
		ObjectSetInteger(0, name, OBJPROP_COLOR, border);
		ObjectSetInteger(0, name, OBJPROP_STYLE, STYLE_SOLID);
		ObjectSetInteger(0, name, OBJPROP_WIDTH, 1);
		ObjectSetInteger(0, name, OBJPROP_BACK, false);
		ObjectSetInteger(0, name, OBJPROP_SELECTABLE, false);
		ObjectSetInteger(0, name, OBJPROP_SELECTED, false);
		ObjectSetInteger(0, name, OBJPROP_HIDDEN, true);
		ObjectSetInteger(0, name, OBJPROP_ZORDER, 1);
	}

	void MakeLabel(string name, int px, int py, string text, int font_size = 10, color text_color = clrRed, string font_name = "Courier New")
	{
		ObjectCreate(name, OBJ_LABEL, 0, 0, 0);
		ObjectSetInteger(0, name, OBJPROP_XDISTANCE,  px);
		ObjectSetInteger(0, name, OBJPROP_YDISTANCE,  py);
		ObjectSetInteger(0, name, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
		ObjectSetString (0, name, OBJPROP_TEXT,       text);
		ObjectSetString (0, name, OBJPROP_TOOLTIP,    "tip: " + text);
		ObjectSetString (0, name, OBJPROP_FONT,       font_name);
		ObjectSetInteger(0, name, OBJPROP_FONTSIZE,   font_size);
		ObjectSetInteger(0, name, OBJPROP_ANCHOR,     ANCHOR_LEFT_UPPER);
		ObjectSetInteger(0, name, OBJPROP_COLOR,      text_color);
		ObjectSetInteger(0, name, OBJPROP_BACK,       false);
		ObjectSetInteger(0, name, OBJPROP_SELECTABLE, false);
		ObjectSetInteger(0, name, OBJPROP_SELECTED,   false);
		ObjectSetInteger(0, name, OBJPROP_HIDDEN,     false);
	}


};