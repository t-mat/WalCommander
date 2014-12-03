/*
	Copyright (c) by Valery Goryachev (Wal)
*/

#ifndef TERMINAL_H
#define TERMINAL_H
#include "wal.h"
#include "shell.h"

#include "t-emulator.h"

using namespace wal;

//#define MAX_TERM_ROWS 1024


#ifdef _WIN32

#else

class TerminalStream {
	int _masterFd;
	carray<char> _slaveName;
public:
	TerminalStream();
	int Read(char* buf, int size);  // Read end Write can work in self threds 
	int Write(char *buf, int size); //
	int SetSize(int r, int c);
	const char *SlaveName(){ return _slaveName.ptr(); }
	~TerminalStream();
};


class TerminalOutputQueue {
	enum { BUFSIZE = 1024 };
	struct Node {
		int pos, size;
		char buf[BUFSIZE];
		Node *next;
		Node() : pos(0), size(0), next(0) {}
	};
	Node *first, *last;
	int count;
public:
	TerminalOutputQueue() : first(0), last(0), count(0){}
	bool Empty() const { return first == 0; }
		
	int Get()
	{
		ASSERT(!Empty());
		if (!first) return -1000;
		ASSERT(first->pos < first->size);
		int c = first->buf[first->pos++];
		if (first->pos >= first->size) 
		{
			Node *p = first;
			first = first->next;
			if (!first) last = 0;
			delete p;
		}
		count--;
		return c;
	}
	
	void Put(int c)
	{
		if (!last) 
		{
			first = last = new Node;
		} else if (last->size>=BUFSIZE) {
			last->next = new Node;
			last = last->next;
		}
		last->buf[last->size++] = c;
		count++;
//		if (c>' ') printf("'%c' ", c); else printf("%i ", c);
	}
	
	void Put(const char *s, int size) { for (; size>0; size--, s++) Put(*s); }
	~TerminalOutputQueue(){}
};


class Terminal {

	friend void* TerminalInputThreadFunc(void *data);
	friend void* TerminalOutputThreadFunc(void *data);
	friend class TerminalWin;
	
	int _rows, _cols;

	TerminalStream _stream;
	Shell _shell;
	
//input 
	Mutex _inputMutex;
//	int _maxRows;

//output
	thread_t outputThread;
	Mutex _outputMutex;
	Cond _outputCond;
	
	TerminalOutputQueue outQueue;
	
	int ReadOutput(char *buf, int size);
	void OutAppendUnicode(const unicode_t c);

	void CharInput(char ch){ _emulator.Append(ch); }
	
//emulator	
	Emulator _emulator; //lock _inputMutex

public:
	Terminal(/*int maxRows = MAX_TERM_ROWS*/);

	Mutex* InputMutex(){ return &_inputMutex; }

private:
	int SetSize(int r, int c);
	unsigned MouseFlags();
public:

	TermChar* Get(int n){ return _emulator.Get(n); } //need lock
	TermChar* operator [](int n){ return Get(n); }
	bool IsChanged(int n){ return _emulator.IsChanged(n); } //need lock
	void SetChanged(int n, bool b){ _emulator.SetChanged(n, b); } //need lock

	void Output(const char c);
	void Output(const char* s, int size);
	void Output(const char* s);
	
	void UnicodeOutput(const unicode_t c);
	void UnicodeOutput(const unicode_t *s, int size);
	Shell& GetShell(){ return _shell; }

	void Key(unsigned key, unsigned ch, unsigned mod);
	
	enum MOUSE_TYPE { MOUSE_PRESS, MOUSE_RELEASE, MOUSE_MOVE };
	enum MOUSE_BUTTON { NO_BUTTON = 0, LEFT=0, CENTER=1, RIGHT=2, WHEEL_FORW=64, WHEEL_BACK=65 };
	
	bool Mouse(MOUSE_TYPE, MOUSE_BUTTON, bool captured, int r, int c, unsigned mod, int rows, int cols);

	void TerminalReset(bool clearScreen = false);
	void TerminalPrint(const unicode_t *str, unsigned fg, unsigned bg);
	
	int CRow(){ return _emulator.ScreenCRow(); }
	int CCol(){ return _emulator.ScreenCCol(); }
	
	int CurrentRows(){ return _emulator.CurrentRows(); } //need lock
        	
	~Terminal(){};
};

void* TerminalInputThreadFunc(void *data);

#endif
#endif
