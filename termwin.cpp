/*
	Copyright (c) by Valery Goryachev (Wal)
*/


#include "termwin.h"
#include "terminal.h"

#ifndef _WIN32

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

void TerminalWin::OnChangeStyles()
{
	wal::GC gc(this);
	gc.Set(GetFont());
	cpoint p = gc.GetTextExtents(ABCString);
	cW = p.x /ABCStringLen;
	cH = p.y;
	if (!cW) cW=10;
	if (!cH) cH=10;

//то же что и в EventSize надо подумать		
	int W = _rect.Width();
	int H = _rect.Height();

	screen.SetSize(H/cH, W/cW);
	Reread();
	CalcScroll();

}

TerminalWin::TerminalWin(int nId, Win *parent)
: Win(WT_CHILD, 0, parent, 0, nId),
	_lo(1,2),
	_scroll(0, this, true, false), //надо пошаманить для автохида
	cH(1),cW(1),
	_firstRow(0),
	_currentRows(1),
	_captureButton(0),
	_captureToMark(false),
	_mouseEnabled(false)
{
	_scroll.Enable();
	_scroll.Show();
	_scroll.SetManagedWin(this);
	_lo.AddWin(&_scroll,0,1);
	_lo.AddRect(&_rect,0,0);
	_lo.SetColGrowth(0);
	SetLayout(&_lo);
	LSRange lr(0,10000,1000);
	LSize ls; ls.x = ls.y = lr;
	SetLSize(ls);

	ThreadCreate(0, TerminalInputThreadFunc, &_terminal);
	OnChangeStyles();
}

int uiClassTerminal = GetUiID("Terminal");

int TerminalWin::UiGetClassId()
{
	return uiClassTerminal; 
}

void TerminalWin::Paste()
{
	ClipboardText ctx;
	ClipboardGetText(this, &ctx);
	int count = ctx.Count();
	if (count <= 0) return;
	for (int i=0; i<count; i++) 
		_terminal.UnicodeOutput(ctx[i]);
}


void TerminalWin::PageUp()
{
	if (SetFirst(_firstRow + (screen.rows - 1)))
	{
		Reread();
		CalcScroll();
		Invalidate();
	}
}


void TerminalWin::PageDown()
{
	if (SetFirst(_firstRow - (screen.rows - 1)))
	{
		Reread();
		CalcScroll();
		Invalidate();
	}
}

bool TerminalWin::SetFirst(int n)
{
	if (n + screen.rows > _currentRows) 
		n = _currentRows - screen.rows;
		
//	if (n + screen.rows > MAX_TERM_ROWS) 
//		n = MAX_TERM_ROWS - screen.rows;

	if (n<0) n = 0;
	
	int old = _firstRow;
	_firstRow = n;

	ASSERT(_firstRow >= 0);
//	ASSERT(_firstRow < _currentRows);	
	
	return n != old;
}


void TerminalWin::Reread()
{
	MutexLock lock(_terminal.InputMutex());
	_terminal.SetSize(screen.rows, screen.cols);
	for (int r = 0; r< screen.rows; r++)
	{
		TermChar *sc = screen.Get(r);
		TermChar *tc = _terminal.Get(r + _firstRow);
		if (tc) 
		{
			memcpy(sc, tc, screen.cols*sizeof(TermChar));
		}
	}
	screen.cursor.Set(_terminal.CRow() - _firstRow,  _terminal.CCol());
}


void TerminalWin::CalcScroll()
{
	ScrollInfo si;
	si.pageSize = screen.rows;
	si.size = _currentRows;
	si.pos = _currentRows - _firstRow - screen.rows;
	bool visible = _scroll.IsVisible();
	_scroll.Command(CMD_SCROLL_INFO, SCMD_SCROLL_VCHANGE, this, &si);

	if (visible != _scroll.IsVisible() ) 
		this->RecalcLayouts();
		
	ASSERT(_firstRow >= 0);
//	ASSERT(_firstRow < MAX_TERM_ROWS);	
}


bool TerminalWin::Command(int id, int subId, Win *win, void *data)
{
	if (id != CMD_SCROLL_INFO) 
		return false;

	int n = _firstRow;
	switch (subId) {
	case SCMD_SCROLL_LINE_UP: n++; break;
	case SCMD_SCROLL_LINE_DOWN: n--; break;
	case SCMD_SCROLL_PAGE_UP: n += screen.rows; break;
	case SCMD_SCROLL_PAGE_DOWN: n -= screen.rows; break;
	case SCMD_SCROLL_TRACK: n = _currentRows - ((int*)data)[0] - screen.rows; break;
	}
	
	if (n + screen.rows > _currentRows) 
		n = _currentRows - screen.rows;

	if (n<0) n = 0;

	if (n != _firstRow) 
	{
		_firstRow = n;
		
		Reread();
		
		CalcScroll();
		Invalidate();
	}

	return true;
}

struct ExecData {
	carray<sys_char_t> cmd;
	carray<sys_char_t> path;
	Shell *shell;
};


void* RunProcessThreadFunc(void *data)
{
	ExecData *p = (ExecData*)data;

	if (p)
	{
		if (!p->shell->CD(p->path.ptr()))
		{			
			pid_t pid = p->shell->Exec(p->cmd.ptr());
			if (pid >0) 
			{
				WinThreadSignal(pid);
				int ret;
//printf("waitpid %i\n", pid);
				int r = p->shell->Wait(pid, &ret);
//printf("done wait %i (ret=%i)\n", pid, r);
				WinThreadSignal(-1);
			}
		}
		delete p;
	}
	return 0;
}


void TerminalWin::Execute(Win*w, int tId, const unicode_t *cmd, const sys_char_t *path)
{
	ExecData *p = new ExecData;
	p->cmd =  unicode_to_sys_array(cmd);
	p->path = new_sys_str(path);
	p->shell = & _terminal.GetShell();
	_terminal.TerminalReset(false);
	w->ThreadCreate(tId, RunProcessThreadFunc, p);
}

void TerminalWin::EventSize(cevent_size *pEvent)
{
	RecalcLayouts();
	cpoint size = pEvent->Size();

	int W = _rect.Width();
	int H = _rect.Height();

	screen.SetSize(H/cH, W/cW);
	Reread();
	CalcScroll();
	Parent()->RecalcLayouts(); //!!!
}

void TerminalWin::EventTimer(int tid)
{
	if (IsCaptured()) 
	{
		EmulatorScreenPoint pt = lastMousePoint;
		int delta  =  (pt.row < 0) ? pt.row : (pt.row >= screen.rows ? pt.row - screen.rows + 1 : 0);
		
		if (delta != 0 && SetFirst(_firstRow + delta*2))
		{
				pt.row += _firstRow;
				screen.marker.Move(pt);
				
				Reread();
				CalcScroll();
				Invalidate();
		}
	}
}

bool TerminalWin::GetMarked(ClipboardText &ct)
{
	ct.Clear();
	if (screen.marker.Empty()) return false;
	MutexLock lock(_terminal.InputMutex());
	int n1 = screen.marker.a.row;
	int n2 = screen.marker.b.row;
	if (n1>n2) { int t = n1; n1 = n2; n2 = t; }
	
	if (n1<0) n1 = 0; 
	if (n2>=_currentRows) n2 = _currentRows -1;
	
	for (int i = n2; i>=n1; i--)
	{
		TermChar *tc = _terminal.Get(i);
		if (i == n1 || i == n2) 
		{
			for (int j = 0; j<screen.cols; j++)
				if (screen.marker.In(EmulatorScreenPoint(i, j))) 
				{
					unicode_t c = tc[j] & 0xFFFF;
					if (c<32) c = 32;
					ct.Append(c);
				}
		} else {
			for (int j = 0; j<screen.cols; j++) 
			{
				unicode_t c = tc[j] & 0xFFFF;
				if (c<32) c = 32;
				ct.Append(c);
			}
		}
		if (i != n1) ct.Append('\n');
	}
}


void TerminalWin::MarkerClear()
{
	if (screen.marker.Empty()) 
		return;
	screen.marker.Reset();
	Invalidate();
}

void TerminalWin::ThreadSignal(int id, int data)
{
//	printf("terminal thread signal id=%i, data=%i\n", id, data);

	MutexLock lock(_terminal.InputMutex());
	
	
	bool calcScroll = false;
	if (_currentRows != _terminal.CurrentRows())
	{
		_currentRows = _terminal.CurrentRows();
		calcScroll = true;
	}
	
	

	wal::GC gc(this);
	gc.Set(GetFont());
	
	bool fullDraw = false;
	
	if (_firstRow !=0 )
	{
		_firstRow = 0;
		CalcScroll();
		fullDraw = true;
	}
	
	if (!screen.marker.Empty())
	{
		screen.marker.Reset();
		fullDraw = true;
	}

	int cols = screen.cols;
	for (int i = 0; i<screen.rows; i++)
		if (_terminal.IsChanged(i))
		{

			TermChar *sc = screen.Get(i);
			TermChar *tc = _terminal.Get(i);
			if (sc && tc)
			{
				int first = 0;
				
				for (; first<cols && sc[first]==tc[first]; first++);
				
				if (first < cols)
				{
					int last = first;

					sc[first] = tc[first];

					for (int j = first+1; j<cols; j++) 
						if (sc[j]!=tc[j]) 
						{
							sc[j] = tc[j];
							last = j;
						}
						
					_terminal.SetChanged(i, false);
					if (!fullDraw) 
					{
						lock.Unlock();
						DrawRow(gc, i, first, last);
						lock.Lock();
					}
				}
			}
			
		}
		
	if (fullDraw)
	{
		Invalidate();
		return;
	}
		
	//cursor
	if (screen.cursor.row>=0 && screen.cursor.row < screen.rows)
	{
		EmulatorScreenPoint cur(_terminal.CRow(), _terminal.CCol());
		lock.Unlock();
		if (screen.cursor != cur) //hide old
			DrawChar(gc, screen.cursor.row, screen.cursor.col, screen.GetChar(screen.cursor.row, screen.cursor.col));

		screen.cursor = cur;
		TermChar ch = screen.GetChar(screen.cursor.row, screen.cursor.col);
		ch = ((ch>>8)&0xFF0000) + ((ch<<8)&0xFF000000) + (ch&0xFFFF);
		DrawChar(gc, screen.cursor.row, screen.cursor.col, ch);
	}
	
	if (calcScroll) CalcScroll();
}

static unsigned default_color_table[] = {
0x000000, 0x000080, 0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0xC0C0C0, 
0x808080, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF,  

0x000000, 0x5f0000, 0x870000, 0xaf0000, 0xd70000, 0xff0000, 0x005f00, 0x5f5f00, //016
0x875f00, 0xaf5f00, 0xd75f00, 0xff5f00, 0x008700, 0x5f8700, 0x878700, 0xaf8700, //024
0xd78700, 0xff8700, 0x00af00, 0x5faf00, 0x87af00, 0xafaf00, 0xd7af00, 0xffaf00, //032
0x00d700, 0x5fd700, 0x87d700, 0xafd700, 0xd7d700, 0xffd700, 0x00ff00, 0x5fff00, //040
0x87ff00, 0xafff00, 0xd7ff00, 0xffff00, 0x00005f, 0x5f005f, 0x87005f, 0xaf005f, //048
0xd7005f, 0xff005f, 0x005f5f, 0x5f5f5f, 0x875f5f, 0xaf5f5f, 0xd75f5f, 0xff5f5f, //056
0x00875f, 0x5f875f, 0x87875f, 0xaf875f, 0xd7875f, 0xff875f, 0x00af5f, 0x5faf5f, //064
0x87af5f, 0xafaf5f, 0xd7af5f, 0xffaf5f, 0x00d75f, 0x5fd75f, 0x87d75f, 0xafd75f, //072
0xd7d75f, 0xffd75f, 0x00ff5f, 0x5fff5f, 0x87ff5f, 0xafff5f, 0xd7ff5f, 0xffff5f, //080
0x000087, 0x5f0087, 0x870087, 0xaf0087, 0xd70087, 0xff0087, 0x005f87, 0x5f5f87, //088
0x875f87, 0xaf5f87, 0xd75f87, 0xff5f87, 0x008787, 0x5f8787, 0x878787, 0xaf8787, //096
0xd78787, 0xff8787, 0x00af87, 0x5faf87, 0x87af87, 0xafaf87, 0xd7af87, 0xffaf87, //104
0x00d787, 0x5fd787, 0x87d787, 0xafd787, 0xd7d787, 0xffd787, 0x00ff87, 0x5fff87, //112
0x87ff87, 0xafff87, 0xd7ff87, 0xffff87, 0x0000af, 0x5f00af, 0x8700af, 0xaf00af, //120
0xd700af, 0xff00af, 0x005faf, 0x5f5faf, 0x875faf, 0xaf5faf, 0xd75faf, 0xff5faf, //128
0x0087af, 0x5f87af, 0x8787af, 0xaf87af, 0xd787af, 0xff87af, 0x00afaf, 0x5fafaf, //136
0x87afaf, 0xafafaf, 0xd7afaf, 0xffafaf, 0x00d7af, 0x5fd7af, 0x87d7af, 0xafd7af, //144
0xd7d7af, 0xffd7af, 0x00ffaf, 0x5fffaf, 0x87ffaf, 0xafffaf, 0xd7ffaf, 0xffffaf, //152
0x0000d7, 0x5f00d7, 0x8700d7, 0xaf00d7, 0xd700d7, 0xff00d7, 0x005fd7, 0x5f5fd7, //160
0x875fd7, 0xaf5fd7, 0xd75fd7, 0xff5fd7, 0x0087d7, 0x5f87d7, 0x8787d7, 0xaf87d7, //168
0xd787d7, 0xff87d7, 0x00afd7, 0x5fafd7, 0x87afd7, 0xafafd7, 0xd7afd7, 0xffafd7, //176
0x00d7d7, 0x5fd7d7, 0x87d7d7, 0xafd7d7, 0xd7d7d7, 0xffd7d7, 0x00ffd7, 0x5fffd7, //184
0x87ffd7, 0xafffd7, 0xd7ffd7, 0xffffd7, 0x0000ff, 0x5f00ff, 0x8700ff, 0xaf00ff, //192
0xd700ff, 0xff00ff, 0x005fff, 0x5f5fff, 0x875fff, 0xaf5fff, 0xd75fff, 0xff5fff, //200
0x0087ff, 0x5f87ff, 0x8787ff, 0xaf87ff, 0xd787ff, 0xff87ff, 0x00afff, 0x5fafff, //208
0x87afff, 0xafafff, 0xd7afff, 0xffafff, 0x00d7ff, 0x5fd7ff, 0x87d7ff, 0xafd7ff, //216
0xd7d7ff, 0xffd7ff, 0x00ffff, 0x5fffff, 0x87ffff, 0xafffff, 0xd7ffff, 0xffffff, //224
0x080808, 0x121212, 0x1c1c1c, 0x262626, 0x303030, 0x3a3a3a, 0x444444, 0x4e4e4e, //232
0x585858, 0x626262, 0x6c6c6c, 0x767676, 0x808080, 0x8a8a8a, 0x949494, 0x9e9e9e, //240
0xa8a8a8, 0xb2b2b2, 0xbcbcbc, 0xc6c6c6, 0xd0d0d0, 0xdadada, 0xe4e4e4, 0xeeeeee, //248
};

void TerminalWin::DrawChar(wal::GC &gc, int r, int c, TermChar tc)
{
	if (r < 0 || r >=screen.rows) return;
	if (c<0 || c>= screen.cols) return;
	gc.SetFillColor(default_color_table[(tc>>24)&0xFF]);
	gc.SetTextColor(default_color_table[(tc>>16)&0xFF]);

	int y = (screen.rows-r-1) * cH;
	int x = c * cW;

	unicode_t ch = tc & 0xFFFF;
	gc.TextOutF(x, y, &ch, 1);
}

#ifndef _WIN32	
Terminal::MOUSE_BUTTON MkTerminalButton(int b)
{
	switch (b) {
	case MB_L: return Terminal::LEFT; 
	case MB_M: return Terminal::CENTER;
	case MB_R: return Terminal::RIGHT;
	default: return Terminal::NO_BUTTON;
	};
}

#endif

bool TerminalWin::EventMouse(cevent_mouse* pEvent)
{
	cpoint coord = pEvent->Point();
	EmulatorScreenPoint pt;
	pt.Set(screen.rows - (coord.y+cH-1)/cH, coord.x / cW);
	bool pointChanged = lastMousePoint != pt;
	lastMousePoint = pt;

	bool shift = (pEvent->Mod() & KM_SHIFT)!=0;
	bool ctrl = (pEvent->Mod() & KM_CTRL)!=0;

	switch (pEvent->Type())	{
	case EV_MOUSE_MOVE:
		if (pointChanged) 
		{
		
#ifndef _WIN32		
			if (!_captureToMark && _mouseEnabled) 
			{
				Terminal::MOUSE_BUTTON button = IsCaptured() ? MkTerminalButton(_captureButton) : Terminal::NO_BUTTON;
	
				if (_terminal.Mouse(Terminal::MOUSE_MOVE, button, IsCaptured(), screen.rows - pt.row - 1, pt.col, pEvent->Mod(), screen.rows, screen.cols))
					break;
			};
#endif			
			
			if (IsCaptured() && pointChanged) 
			{
				pt.row += _firstRow;
				screen.marker.Move(pt);
				Invalidate();
			}
		}
		break;
		
	case EV_MOUSE_WHEEL:
		{
#ifndef _WIN32		
			if (!shift && !_captureToMark && _mouseEnabled) 
			{
				Terminal::MOUSE_BUTTON button = (pEvent->Delta() > 0) ?  Terminal::WHEEL_FORW : Terminal::WHEEL_BACK;
				if (_terminal.Mouse(Terminal::MOUSE_PRESS, button, IsCaptured(), screen.rows - pt.row - 1, pt.col, pEvent->Mod(), screen.rows, screen.cols))
					break;
			};
#endif			
		
		
			if (pEvent->Delta() > 0) 
			{
				PageUp();
				
				if (IsCaptured()) 
				{
					pt.row += _firstRow;
					screen.marker.Move(pt);
					Invalidate();
				}

				break;
			}

			if (pEvent->Delta() < 0) 
			{
				PageDown();
				
				if (IsCaptured()) 
				{
					pt.row += _firstRow;
					screen.marker.Move(pt);
					Invalidate();
				}
				
				break;
			}
		}
		break;	
		
	case EV_MOUSE_DOUBLE:
	case EV_MOUSE_PRESS:
		{
			if (IsCaptured()) 
				break;
				
//			if (pEvent->Button()!=MB_L) 
//				break;
			
			_captureButton = pEvent->Button();
			_captureToMark = shift;
			SetCapture();

#ifndef _WIN32		
			if (!_captureToMark && _mouseEnabled) 
			{
				Terminal::MOUSE_BUTTON button = MkTerminalButton(pEvent->Button());
				if (_terminal.Mouse(Terminal::MOUSE_PRESS, button, IsCaptured(), screen.rows - pt.row - 1, pt.col, pEvent->Mod(), screen.rows, screen.cols))
					break;
			};
#endif			
						
			
			SetTimer(1, 100);
			pt.row += _firstRow;
			screen.marker.Set(pt);
			Invalidate();
		}
		break;
		
	case EV_MOUSE_RELEASE:
		if (pEvent->Button() != _captureButton)	break;

#ifndef _WIN32		
		if (!_captureToMark && _mouseEnabled) 
		{
			Terminal::MOUSE_BUTTON button = MkTerminalButton(_captureButton);
			_terminal.Mouse(Terminal::MOUSE_RELEASE, button, IsCaptured(), screen.rows - pt.row - 1, pt.col, pEvent->Mod(), screen.rows, screen.cols);
		};
#endif			
		_captureToMark = false;	
		
		DelTimer(1);
		ReleaseCapture();
		_captureButton = 0;
		break;
	};
	
	return false;
}


void TerminalWin::DrawRow(wal::GC &gc, int r, int first, int last)
{
	int rows = screen.rows;
	if (r < 0 || r  >= rows) return;
	int cols = screen.cols;
	if (first >= cols) first = cols-1;
	if (first < 0) first=0;
	if (last >= cols) last = cols-1;
	if (last < 0) last=0;
	if (first > last) return;

	int y = (rows - r  -1) * cH;
	int x = first * cW;

	ASSERT(r>=0 && r<MAX_TERM_ROWS);
	TermChar *p =  screen.Get(r);

	ScreenMarker m = screen.marker;
	int absRow = _firstRow + r;
	
	while (first <= last) 
	{	
		int n = 1;
		
		bool marked = m.In(EmulatorScreenPoint(absRow, first));
		
		for (int i = first+1; i<=last; i++, n++) 
		{ 
			if ( (p[i] & 0xFFFF0000) != (p[i-1] & 0xFFFF0000) || 
				m.In(EmulatorScreenPoint(absRow,i)) != marked) 
				break; 
		}

		unsigned c = ((p[first]>>16) & 0xFFFF);
		
		if (marked)
		{
			c = 0xFF00;
			
			//c = ((c>>4)|(c<<4))&0xFF;
		}

		gc.SetFillColor(default_color_table[(c>>8) & 0xFF]);
		gc.SetTextColor(default_color_table[c & 0xFF]);
		
		while (n>0) 
		{
			int count = n;
			unicode_t buf[0x100];
			if (count > 0x100) count = 0x100;
			for (int i = 0; i<count; i++) 
			{
				buf[i] = (p[first+i] & 0xFFFF);
				
			}
			gc.TextOutF(x,y,buf, count);
			n-=count;
			first+=count;
			x+=count*cW;
		}
	}
}

void TerminalWin::Paint(wal::GC &gc, const crect &paintRect)
{
	crect r = ClientRect();

	gc.Set(GetFont());
	for (int i = 0; i < screen.rows;i++) 
		DrawRow(gc,i,0,screen.cols-1);

//cursor 
	if (screen.cursor.row>=0 && screen.cursor.row < screen.rows)
	{
		TermChar ch = screen.GetChar(screen.cursor.row, screen.cursor.col);
		ch = ((ch>>8) & 0xFF0000) + ((ch<<8) & 0xFF000000) + (ch & 0xFFFF);
		DrawChar(gc, screen.cursor.row, screen.cursor.col, ch);
	}

	crect r1 = r;
	r1.top = screen.rows*cH;
	gc.SetFillColor(0); //0x200000);
	gc.FillRect(r1);

	r1 = r;
	r1.left = screen.cols*cW;
	gc.FillRect(r1);
}

TerminalWin::~TerminalWin(){};
#endif
