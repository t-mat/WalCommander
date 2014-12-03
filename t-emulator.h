#ifndef T_EMULATOR_H
#define T_EMULATOR_H

#include "wal.h"


using namespace wal;

//BBFFCCCC
typedef unsigned TermChar;


class EmulatorCLList {
	int dataSize;
	carray<bool> data;
	int count;
public:
	EmulatorCLList():dataSize(0x100), data(0x100), count(0){}
	void SetAll(bool b){ for (int i = 0; i<count; i++) data[i]=b; }
	void SetSize(int size){ if (dataSize < size){ data.alloc(size); dataSize = size; } count = size; SetAll(true); }
	void Set(int n, bool b){ if (n>=0 && n<count) data[n] = b; }
	bool Get(int n){ return (n>=0 && n<count) ? data[n] : false; }
};


struct EmulatorScreenPoint {
	int row;
	int col; 
	EmulatorScreenPoint() : row(0), col(0) {}
	EmulatorScreenPoint(int r, int c) : row(r), col(c) {}
	EmulatorScreenPoint(const EmulatorScreenPoint &a):row(a.row), col(a.col){}
	void Set(int r, int c){ row = r; col = c; }

	//нумерация строк обратная, поэтому и такая ботва со сравнением колонок
	bool operator <  (const EmulatorScreenPoint &a) const { return row < a.row || row == a.row && col > a.col; }
	bool operator <= (const EmulatorScreenPoint &a) const { return row < a.row || row == a.row && col >= a.col; }
	
	bool operator != (const EmulatorScreenPoint &a) const { return row != a.row || col != a.col; }
	bool operator == (const EmulatorScreenPoint &a) const { return row == a.row && col == a.col; }
};


inline void ClearEmulatorLine(TermChar *p, int count, unsigned ch = ' '|0x0F00000)
{
	for (;count>0; count--) *(p++) = ch;
}


class EmulatorScreen {
	int rows;
	int cols;
	int lineCount;
	int lineSize;
	EmulatorCLList *clList;
	carray<carray<TermChar> > list;
	int CLN(int n){ return n>=rows ? rows-1 : (n<0 ? 0 : n); }
	void SetCL(int n){ if (clList) clList->Set(n, true); }
public:
	EmulatorScreen(int r, int c, EmulatorCLList *cl);
	int Rows() const { return rows; }
	int Cols() const { return cols; }
	void Clear();
	void SetSize(int r, int c);
	TermChar* Get(int n){ return n>0 && n<rows ? list[n].ptr() : list[0].ptr(); }
	void ScrollUp(int a, int b, int count, unsigned ch); //a<=b
	void ScrollDown(int a, int b, int count, unsigned ch); //a<=b
	void SetLineChar(int ln, int c, int count, unsigned ch);
	void SetLineChar(int ln, int c, unsigned ch);
	void InsertLineChar(int ln, int c, int count, unsigned ch);
	void DeleteLineChar(int ln, int c, int count, unsigned ch);
};

#define DEF_BG_COLOR  0
#define DEF_FG_COLOR  8

struct EmulatorAttr {
	bool cursorVisible;
	bool cursorBlinked;
	unicode_t *G[4];
	unicode_t nG;
	
	int  tabSize;
	bool bold;
	bool blink;
	bool inverse;
	bool underscore;
	unsigned fColor;
	unsigned bColor;

		
	EmulatorAttr(): 
		cursorVisible(true),
		cursorBlinked(true),
		nG(0),
		tabSize(8), bold(false), blink(false), 
		inverse(false), underscore(false),
		fColor(DEF_FG_COLOR), bColor(DEF_BG_COLOR)
	{ 
		G[0]=G[1]=G[2]=G[3]=0;
	}

	void Reset() 
	{		
		tabSize = 8;
		bold = blink = inverse = underscore = false;
		
		fColor = DEF_FG_COLOR;
		bColor = DEF_BG_COLOR;
		
		G[0]=G[1]=G[2]=G[3]=0;
		nG = 0;
		cursorVisible = true;
		cursorBlinked = true;
	}
			
	unsigned Color() 
	{
		unsigned color = inverse ?  (fColor<<24) | (bColor<<16) : (fColor<<16) | (bColor<<24);
		
		if ((bold || blink) && !inverse && fColor < 8)
		{
			color |= 0x080000;
		}
		
		return color;
	};
	
	unicode_t GetSymbol(unsigned char ch){ return G[nG] ? G[nG][ch] : ch; }
	
	void SetNormal()
	{
		fColor = DEF_FG_COLOR;
		bColor = DEF_BG_COLOR;

		bold =  blink = underscore =  inverse = false; 
	}
};


class Emulator {
public:

	
	enum MOUSE_FLAGS {
		MF_9 	= 0x0001, //X10_MOUSE
		MF_1000	= 0x0002, //VT200_MOUSE
		MF_1001	= 0x0004, //VT200_HIGHLIGHT_MOUSE 1001
		MF_1002	= 0x0008, //BTN_EVENT_MOUSE 1002
		MF_1003	= 0x0010, //ANY_EVENT_MOUSE 1003
		MF_1005 = 0x0020, //EXT_MODE_MOUSE 1005
		MF_1006 = 0x0040, //SGR_EXT_MODE_MOUSE 1006
		MF_1015 = 0x0080 //URXVT_EXT_MODE_MOUSE 1015
	};


protected:

	struct Cursor {
		int row;
		int col;
		Cursor():row(0),col(0){}
		Cursor(int r, int c):row(r), col(c){}
		void Set(int r, int c){ row = r; col = c; }
	};


	template <int SIZE = 32> struct NumberList {
		int list[SIZE];
		int count;
		void Reset(){ count = 0; for (int i =0; i<SIZE; i++) list[i] = 0; }
		NumberList(){ Reset(); }
		bool Read(char c){ if (c<'0' || c>'9') return false; list[count] = list[count]*10 + (c-'0'); return true; }
		bool ReadList(char c, char delimiter = ';') { if (Read(c)) return true; if(c != delimiter) return false; if (count<SIZE) count++; return true; }
		int operator[](int n){ ASSERT(n >= 0 && n < SIZE); return list[n]; }
	};

	enum Keypad {
		K_NORMAL =0,
		K_APPLICATION
	} ;

	
	EmulatorScreen _screen0;
	EmulatorScreen _screen1;
	EmulatorScreen *_screen;
	EmulatorCLList _clList;
	void Changed(int n){_clList.Set(_rows-n, true); }

	
	int _rows;
	int _cols;
	int _scT;
	int _scB;
	EmulatorAttr _attr;
	bool _wrap;
	Cursor _cursor;
	Keypad _keypad;
	int _utf8count;
	int _utf8char;
	int _state;
	NumberList<> _N;
	ccollect<char, 0x100> _TXT;
	
	EmulatorAttr _savedAttr;
	Cursor _savedCursor;

	unsigned _mouseFlags;
	
	void ScrollUp(int n);
	void ScrollDown(int n);
	
public:
	void ResetState();
	Emulator();
	void EraseDisplays();
	TermChar* Get(int n){ return _screen->Get(n); }
	bool IsChanged(int n){ return _clList.Get(n); }
	void SetChanged(int n, bool b){ return _clList.Set(n, b); }
	int SetSize(int r, int c);
	void AddUnicode(unicode_t ch);
	unsigned MouseFlags() const { return _mouseFlags; }
	bool KbIsNormal(){ return _keypad == K_NORMAL; }
	void AddCh(char ch);
	
	void SetCursor(int r, int c);
	void IncCursor(int r, int c);
	void EraseDisp(int mode);
		
	void CR();
	void LF();
	void Tab();
	void Reset(bool clearScreen);
	void IND();
	void RI();
	void RestoreCursor();
	
	int ScreenCRow(){ return _rows-_cursor.row-1; }
	int ScreenCCol(){ return _cursor.col; }
	int CurrentRows(){ return _screen->Rows(); }
	
	void InternalPrint(const unicode_t *str, unsigned fg, unsigned bg);
	void Append(char ch);
};

#endif
