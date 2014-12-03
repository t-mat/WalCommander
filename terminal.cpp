/*
	Copyright (c) by Valery Goryachev (Wal)
*/


#include "wal.h"
#include "swl.h"
#include "terminal.h"
#include <termios.h>
#include "wcm-config.h"
#include <sys/ioctl.h>

#ifdef _WIN32
#error
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

TerminalStream::TerminalStream()
:	_masterFd(-1)
{
	char masterName[0x100];
	strcpy(masterName, "/dev/ptmx");

	char slaveName[0x100];

	_masterFd = open(masterName, O_RDWR | O_NDELAY);

	if (_masterFd >= 0) 
	{
		char *name = ptsname(_masterFd);
		if (!name) 
			throw_syserr(0,"INTERNAL ERROR: can`t receive slave name for %s", masterName);
		strcpy(slaveName, name);
	} else { 
		//for old freeBsd
        	static char p1[]= "prsPRS";
		static char p2[]= "0123456789ancdefghijklmnopqrstuv";

		for (char *s1 = p1; *s1; s1++)
		{
			for (char *s2 = p2; *s2; s2++)
			{
				sprintf(masterName, "/dev/pty%c%c", *s1, *s2);
				_masterFd = open(masterName, O_RDWR | O_NDELAY);
				if (_masterFd>=0) 
				{
					sprintf(slaveName, "/dev/tty%c%c", *s1, *s2);
					 goto Ok;
				}
			}
		}

		throw_syserr(0,"INTERNAL ERROR: can`t open any  pseudo terminal file");
		Ok:;
	}

	if (grantpt(_masterFd))
		throw_syserr(0,"INTERNAL ERROR: can`t grant  pseudo terminal (%s)", masterName);
		
	if (unlockpt(_masterFd))
		throw_syserr(0,"INTERNAL ERROR: can`t unlock  pseudo terminal (%s)", masterName);


	_slaveName = new_sys_str(slaveName);
}


static int WritePipe(int fd, int cmd, ...)
{
	ccollect<char*> list;

	va_list ap;
	va_start(ap, cmd);
	
	while (true) 
	{
		char *s = va_arg(ap, char*);
		if (!s) break;
		list.append(s);
	}
	
	va_end(ap);
	
	int ret;
	char c = cmd;
	ret = write(fd, &c, sizeof(char)); if (ret<0 || ret != sizeof(char)) return -1;
	c = list.count();
	ret = write(fd, &c, sizeof(char)); if (ret<0 || ret != sizeof(char)) return -1;
	
	for (int i = 0; i<list.count(); i++)
	{
		char *s = list[i];
		int l = strlen(s)+1;
		ret = write(fd, s, l); if (ret<0 || ret != l) return -1;
	}
	return 0;
}

static int ReadPipe(int fd, int &cmd, ccollect<carray<char> > &params)
{
	char c;
	int ret;
	ret = read(fd, &c, sizeof(char)); if (ret<0 || ret != sizeof(char)) return -1;
	cmd = c;
	ret = read(fd, &c, sizeof(char)); if (ret<0 || ret != sizeof(char)) return -1;
	params.clear();
	
	int count = c;
	for (int i = 0; i < count; i++)
	{
		ccollect<char> p;
		while (true) 
		{
			ret = read(fd, &c, sizeof(char)); if (ret<0 || ret != sizeof(char)) return -1;
			p.append(c);
			if (!c) break;
		}
		params.append(p.grab());
	}
	
	return 0;
}

	
static void Shell(int in, int out)
{
	fcntl(in, F_SETFD, long(FD_CLOEXEC));
	fcntl(out, F_SETFD, long(FD_CLOEXEC));
	

	while (true) 
	{
		ccollect<carray<char> > pList;
		int cmd = 0;
		
		if (ReadPipe(in, cmd, pList)) exit(1);
		
		switch (cmd) {
		case 1: 

			{
				pid_t pid = fork();
				if (!pid) 
				{
					static char shell[]="/bin/sh";
					if (pList.count()) 
					{
						const char * params[]={shell, "-c", pList[0].ptr(), NULL};
						execv(shell, (char **) params);
						printf("error execute %s\n",shell);
					} else 
						printf("internal err (no shall paremeters)\n");
					
					exit(1);
				}
				char buf[64];
				sprintf(buf,"%i", int(pid));
				if (WritePipe(out, 0, buf, (const char*) 0)) exit(1);
				
printf ("exec '%s' (%i)\n", pList[0].ptr(), pid);

			}
			break;
		default: 
			if (WritePipe(out, 1, "unknown internal shell command", (const char*)0 )) exit(1);
			
			printf("internal err (unknown shell command)\n");
			
			break;
		};
	}
}


int TerminalStream::Read(char* buf, int size)
{
	fd_set fr;
	FD_ZERO(&fr);
	FD_SET(_masterFd, &fr);
	int n = select(_masterFd + 1, &fr, 0, 0, 0);

//printf("select returned %i (_masterFd = %i)\n", n, _masterFd);

	if (n < 0) return n;
	n = read(_masterFd, buf, size);

//printf("terminal readed %i bytes\n", n);

	return n;
}

int TerminalStream::Write(char *buf, int size)
{
	int bytes = size;
	char *p = buf;
	while (bytes > 0) 
	{
		fd_set fr;
		FD_ZERO(&fr);
		FD_SET(_masterFd, &fr);
		int n = select(_masterFd + 1, 0, &fr, 0, 0);
		if (n < 0) return n;
		n = write(_masterFd, p, bytes);
		if (n < 0) return n;
		p += n;
		bytes -= n;
	}
	return size;
}

int TerminalStream::SetSize(int rows, int cols)
{
	struct winsize ws;
	ws.ws_row = (rows>0)? rows:1;
	ws.ws_col = (cols>0)? cols:1;
	int r = ioctl( _masterFd, TIOCSWINSZ, &ws);
	return 0;
}

TerminalStream::~TerminalStream()
{
	if (_masterFd >= 0) 
	{
		close(_masterFd);
		_masterFd = -1; 
	}
}

void* TerminalInputThreadFunc(void *data);
void* TerminalOutputThreadFunc(void *data);

Terminal::Terminal(/*int maxRows*/)
:	
	_rows(10), 
	_cols(10), 
//	_maxRows(maxRows), 
	_stream(),
	_shell(_stream.SlaveName())
{
	_emulator.SetSize(_rows, _cols);
	int err = thread_create(&outputThread, TerminalOutputThreadFunc, this);
	if (err) throw_syserr(err,"INTERNAL can`t create thread (Terminal::Terminal)");
}



/////////////////////////////////////////////////////////////////////////////////////////
	int Terminal::SetSize(int r, int c)
	{
		_rows = r;
		_cols = c;
		_emulator.SetSize(_rows, _cols);
		_stream.SetSize(r, c);
		return 0;
	}

#define ESC "\x1b"
	


void Terminal::Key(unsigned key, unsigned ch, unsigned mod)
{
	const char *m = "";
	
	if (mod & KM_SHIFT) {
		if (mod & KM_CTRL)
			if (mod & KM_ALT) m = ";8"; else m = ";6";
		else 
			if (mod & KM_ALT) m = ";4"; else m = ";2";
	} else {
		if (mod & KM_CTRL) 
			if (mod & KM_ALT) m = ";7"; else m = ";5";
		else
			if (mod & KM_ALT) m = ";3";
	};
	
	char ec = 0;
	
	switch (key) {
	case VK_F1:	ec = 'P'; if (*m) goto CSI_1_mod; goto SS3;
	case VK_F2:	ec = 'Q'; if (*m) goto CSI_1_mod; goto SS3;
	case VK_F3:	ec = 'R'; if (*m) goto CSI_1_mod; goto SS3;
	case VK_F4:	ec = 'S'; if (*m) goto CSI_1_mod; goto SS3;

	case VK_F5:	Output("\x1b[15"); Output(m); Output('~'); return;
	case VK_F6:	Output("\x1b[17"); Output(m); Output('~'); return;
	case VK_F7:	Output("\x1b[18"); Output(m); Output('~'); return;
	case VK_F8:	Output("\x1b[19"); Output(m); Output('~'); return;
	case VK_F9:	Output("\x1b[20"); Output(m); Output('~'); return;
	case VK_F10:	Output("\x1b[21"); Output(m); Output('~'); return;
	case VK_F11:	Output("\x1b[23"); Output(m); Output('~'); return;
	case VK_F12:	Output("\x1b[24"); Output(m); Output('~'); return;

	case VK_UP:	ec = 'A'; if (*m) goto CSI_1_mod; if (_emulator.KbIsNormal()) goto CSI; goto SS3;
	case VK_DOWN:	ec = 'B'; if (*m) goto CSI_1_mod; if (_emulator.KbIsNormal()) goto CSI; goto SS3;
	case VK_RIGHT:	ec = 'C'; if (*m) goto CSI_1_mod; if (_emulator.KbIsNormal()) goto CSI; goto SS3;
	case VK_LEFT:	ec = 'D'; if (*m) goto CSI_1_mod; if (_emulator.KbIsNormal()) goto CSI; goto SS3;
	case VK_HOME:	ec = 'H'; if (*m) goto CSI_1_mod; if (_emulator.KbIsNormal()) goto CSI; goto SS3;
	case VK_END:	ec = 'F'; if (*m) goto CSI_1_mod; if (_emulator.KbIsNormal()) goto CSI; goto SS3;

	case VK_BACK: Output(wcmConfig.terminalBackspaceKey ? 8 : 127); return; //херово конечно без блокировок обращаться, но во время работы терминала конфиг не меняется
	case VK_DELETE:	Output("\x1b[3"); Output(m); Output('~'); return;
	case VK_NEXT:	Output("\x1b[6"); Output(m); Output('~'); return;
	case VK_PRIOR:	Output("\x1b[5"); Output(m); Output('~'); return;
	case VK_INSERT:	Output("\x1b[2"); Output(m); Output('~'); return;
	
	case VK_TAB: 
		if (mod &KM_SHIFT) { 
			Output("\x1b[Z"); 	
			return; 
		}; 
		break;

	}

	if (!ch) return;
	
	UnicodeOutput(ch);
	return;
SS3:
	Output(ESC "O"); Output(ec);
	return;

CSI:
	Output(ESC "["); Output(ec);
	return;
	
CSI_1_mod:
	Output(ESC "[1"); Output(m);  Output(ec);
	return;
	
}

inline char* to_utf8(char *s, unsigned c)
{
	if (c < 0x800)
	{
		if (c < 0x80)
			*(s++) = c;
		else {
			*(s++) = (0xC0 | (c>>6));
			*(s++) = 0x80 | (c & 0x3F); 
		}
	} else {
		if (c < 0x10000) //1110xxxx 10xxxxxx 10xxxxxx
		{
			s[2] = 0x80 | c & 0x3F; c>>=6;
			s[1] = 0x80 | c & 0x3F; c>>=6;
			s[0] = (c & 0x0F) | 0xE0;
			s +=3;
		} else { //11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			s[3] = 0x80 |c & 0x3F; c>>=6;
			s[2] = 0x80 | c & 0x3F; c>>=6;
			s[1] = 0x80 | c & 0x3F; c>>=6;
			s[0] = (c & 0x7) | 0xF0;
			s += 4;
		}
	}
	return s;
}



bool Terminal::Mouse(MOUSE_TYPE type, MOUSE_BUTTON button, bool captured, int r, int c, unsigned mod, int rows, int cols)
{
	unsigned flags = MouseFlags();

	if ( !  (
			( flags & Emulator::MF_1003 ) ||
			( flags & Emulator::MF_1002 ) && (type == MOUSE_PRESS || type == MOUSE_RELEASE || type == MOUSE_MOVE && captured) ||
			( flags & Emulator::MF_1000 ) && (type == MOUSE_PRESS || type == MOUSE_RELEASE) ||
			( flags & Emulator::MF_9    ) && type == MOUSE_PRESS 
		)
	) return false;
	
	int bt = button;

	if (type == MOUSE_RELEASE) 
		bt = 3;
	else 
		if (type == MOUSE_MOVE) 
			bt |= 32;
	

	//4=Shift, 8=Meta, 16=Control
	if (mod & KM_SHIFT) bt |= 4;
	if (mod & KM_META) bt |= 8;	
	if (mod & KM_CTRL) bt |= 16;
	
	if (r>=rows) r = rows-1;
	if (c>=cols) c = cols-1;
	
	if (r<0) r = 0;
	if (c<0) c = 0;
	
	r++;
	c++;

	bt += 32;
	
	char buf[0x100] = "\x1b[";
	char *s = buf + 2;
	
	if ( flags & Emulator::MF_1015 ) //URXVT
	{
		s = positive_to_char_decimal<int>(bt, s);
		*(s++) = ';';
		s = positive_to_char_decimal<int>(c, s);
		*(s++) = ';';
		s = positive_to_char_decimal<int>(r, s);
		*(s++) = 'M';
	} else {
		r += 32;
		c += 32;
	
		*(s++) = 'M';
		*(s++) =  bt;
	
	
		if ( flags & Emulator::MF_1005 ) { //utf8
			s = to_utf8(s, c);
			s = to_utf8(s, r);
		} else 
		if ( flags & Emulator::MF_1006 ) { //SGR_EXT_MODE_MOUSE 1006
			//...
			return false;
		} else {
			if (r>255) r = 255;
			if (c>255) c = 255;
			*(s++) = c;
			*(s++) = r;
		}
	}
	
	Output(buf, s - buf);
	
	return true;
}


int Terminal::ReadOutput(char *buf, int size)
{
	int i;
	for (i=0; i<size && !outQueue.Empty(); i++)
		buf[i] = outQueue.Get();
		
	return i;
}

void Terminal::Output(const char c)
{
	MutexLock lock(&_outputMutex);
	outQueue.Put(c);
	_outputCond.Signal();
}

void Terminal::Output(const char* s, int size)
{
	if (size<=0) return;
	MutexLock lock(&_outputMutex);
	outQueue.Put(s, size);
	_outputCond.Signal();
}

void Terminal::Output(const char* s)
{
	Output(s, strlen(s));
}


inline void Terminal::OutAppendUnicode(unicode_t c)
{
	if (!c) return;

//пока в utf8	
		
	if (c < 0x800)
	{
		if (c < 0x80)
			outQueue.Put(c);
		else {
			outQueue.Put((0xC0 | (c>>6)));
			outQueue.Put(0x80 | (c & 0x3F)); 
		}
	} else {
		if (c < 0x10000) //1110xxxx 10xxxxxx 10xxxxxx
		{
			outQueue.Put(0x80 | c & 0x3F); c>>=6;
			outQueue.Put(0x80 | c & 0x3F); c>>=6;
			outQueue.Put((c & 0x0F) | 0xE0);
		} else { //11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			outQueue.Put(0x80 |c & 0x3F); c>>=6;
			outQueue.Put(0x80 | c & 0x3F); c>>=6;
			outQueue.Put(0x80 | c & 0x3F); c>>=6;
			outQueue.Put((c & 0x7) | 0xF0);
		}
	}
}

void Terminal::UnicodeOutput(const unicode_t c)
{
	MutexLock lock(&_outputMutex);
	OutAppendUnicode(c);
	_outputCond.Signal();
}

void Terminal::UnicodeOutput(const unicode_t *s, int size)
{
	if (size<=0) return;
	MutexLock lock(&_outputMutex);
	for (;size>0;size--,s++) OutAppendUnicode(*s);
	_outputCond.Signal();
}


unsigned Terminal::MouseFlags()
{
	MutexLock lock(&_inputMutex);
	return _emulator.MouseFlags();
}

void Terminal::TerminalReset(bool clearScreen)
{
	MutexLock lock(&_inputMutex);
	_emulator.Reset(clearScreen);
}

void Terminal::TerminalPrint(const unicode_t *str, unsigned fg, unsigned bg)
{
	MutexLock lock(&_inputMutex);
	_emulator.InternalPrint(str, fg, bg);
}

#include <stdarg.h>


#define  DBG dbg_printf
//#define  DBG printf


void* TerminalInputThreadFunc(void *data)
{
	Terminal *terminal = (Terminal*)data;

	while (true) 
	{	
		while (true) 
		{
			char buffer[1024];
			int n = terminal->_stream.Read(buffer, sizeof(buffer));
			if (n<0) {
				//...
				break;
			}
			
			if (!n) {
				break; //eof
			}
			
			MutexLock lock(&terminal->_inputMutex);
			for (int i = 0; i<n; i++) 
				terminal->CharInput(buffer[i]);
			WinThreadSignal(0);
		}
		
		//скорее всего shell сломался (перезапустится), ждем 1 секунду
		sleep(1);
	}
}

void* TerminalOutputThreadFunc(void *data)
{
	Terminal *terminal = (Terminal*)data;
	while (true) 
	{
		while (true) 
		{
			terminal->_outputMutex.Lock();
			char buffer[0x100];
			int bytes = terminal->ReadOutput(buffer, sizeof(buffer));
	
			if (bytes<=0) 
			{
				terminal->_outputCond.Wait(&terminal->_outputMutex);
				terminal->_outputMutex.Unlock();
				continue;
			}
	
			terminal->_outputMutex.Unlock();
	
			char *p = buffer;
        	        while (bytes > 0) 
			{
				int n = terminal->_stream.Write(p, bytes);
				if (n<0) {	
					//...
					break;
				}
				p+=n;
				bytes-=n;
			}
		}
		
		//скорее всего shell сломался (перезапустится), ждем 1 секунду
		sleep(1);
	}
	return 0;
}


