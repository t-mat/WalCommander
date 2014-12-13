#ifndef TSCREEN_H
#define TSCREEN_H

#include "t-emulator.h" //для EmulatorScreenPoint :(

struct ScreenMarker {
	EmulatorScreenPoint a, b;
	ScreenMarker(){}
	void Set(int r, int c){ a.Set(r,c); b = a; }
	void Set(const EmulatorScreenPoint & p){ a=b=p;};
	void Reset(){ a=b; }
	bool Empty() const  { return a==b; }
	void Move(int r, int c){ b.Set(r, c); }
	void Move(const EmulatorScreenPoint & p){ b=p;};
	
	bool In(const EmulatorScreenPoint &p) const { return (a < b) ? (a <= p && p < b) : (b < p && p <= a); }
};

struct TScreen {
	int rows, cols;
	EmulatorScreenPoint cursor;
	ScreenMarker marker;
	
	int bufSize; 
	carray<TermChar> buf;
	TScreen():rows(1),cols(1),buf(1), bufSize(1){}

	void SetSize(int r, int c)
	{
		int newSize = r*c;
		if (bufSize < newSize)
		{
			carray<TermChar> p(newSize);
			buf = p;
			bufSize = newSize;
		}
		rows = r; cols = c;
		memset(buf.ptr(),0,bufSize*sizeof(TermChar));
	}

	TermChar* Get(int r){ return buf.ptr()+r*cols; }
	TermChar GetChar(int r, int c){ return (r>=0 && r<rows && c>=0 && c<cols) ? Get(r)[c]:' '; }
	
};


#endif
