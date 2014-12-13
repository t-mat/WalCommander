/*
	Copyright (c) by Valery Goryachev (Wal)
*/

#ifndef TERMWIN_H
#define TERMWIN_H

#include "nc.h"

#include "terminal.h"
#include "tscreen.h"

extern int uiClassTerminal;


class TerminalWin : public Win {
	friend void* RunProcessThreadFunc(void *data);
	int _currentRows;
	
#ifdef _WIN32

#else
	Terminal _terminal;
#endif
	Layout _lo;
	ScrollBar _scroll;
	TScreen screen; //это ТОЛЬКО видимая область экрана
	int cH, cW; 
	crect _rect;
	int _firstRow; 
	int _captureButton;
	bool _captureToMark;
	bool _mouseEnabled;
	
	void Reread();
	void DrawRow(wal::GC &gc, int r, int first, int last);
	void DrawChar(wal::GC &gc, int r, int c, TermChar ch);
	void CalcScroll();
	bool SetFirst(int n);
	
	EmulatorScreenPoint lastMousePoint;
public:
	TerminalWin(int nId, Win *parent);
	virtual void Paint(wal::GC &gc, const crect &paintRect);
	virtual void ThreadSignal(int id, int data);
	virtual void EventSize(cevent_size *pEvent);
	virtual bool EventMouse(cevent_mouse* pEvent);
	virtual void EventTimer(int tid);
	
	void MouseEnable(bool enable = true){ _mouseEnabled = enable; }
	void MouseDisable(){ MouseEnable(false); }
	
	virtual ~TerminalWin();
	void Execute(Win*w, int tId, const unicode_t *cmd, const sys_char_t *path);
#ifdef _WIN32
	void Key(unsigned key, unsigned ch){}
	void TerminalReset(bool clearScreen = false){ }
	void TerminalPrint(const unicode_t *s, unsigned fg, unsigned bg){ }
#else
	void Key(unsigned key, unsigned ch, unsigned mod){ _terminal.Key(key, ch, mod); }
	void TerminalReset(bool clearScreen = false){ _terminal.TerminalReset(clearScreen); }
	void TerminalPrint(const unicode_t *s, unsigned fg, unsigned bg){ _terminal.TerminalPrint(s, fg, bg); }
#endif
	void Paste();
	void PageUp();
	void PageDown();
	bool Marked() const { return !screen.marker.Empty(); }
	bool GetMarked(ClipboardText &ct);
	void MarkerClear();
	virtual bool Command(int id, int subId, Win *win, void *data);
	virtual int UiGetClassId();
	virtual void OnChangeStyles();
};


#endif
