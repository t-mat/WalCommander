#include <wal.h>
using namespace wal;
#include "config-util.h"
#include "string-util.h"
#include "vfs.h"
#include "ltext.h"
#include "globals.h"

#ifdef _WIN32
#include "w32util.h"
#endif



#ifndef _WIN32

#define DEFAULT_CONFIG_PATH UNIX_CONFIG_DIR_PATH "/config.default"

class TextInStream {
	int bufSize;
	carray<char> buffer;
	int pos, count;
	bool FillBuffer(){ if (pos < count) return true; if ( count <= 0) return false; count = Read(buffer.ptr(), bufSize); pos = 0; return count>0; }
public:
	TextInStream(int _bSize = 1024):bufSize(_bSize>0 ? _bSize : 1024), count(1), pos(1){ buffer.alloc(bufSize); }
	int GetC(){ FillBuffer(); return pos<count ? buffer[pos++] : EOF; }
	bool GetLine(char *s, int size);
	~TextInStream(){}
protected:
	virtual int Read(char *buf, int size) = 0; //can throw
};


bool TextInStream::GetLine(char *s, int size) {
	if (size <= 0) return true;
	if (!FillBuffer()) return false;
	if (size == 1) { *s=0; return true; }
	size--;
		
	while (true) {
		if (pos >= count && !FillBuffer()) break;
		char *b = buffer.ptr() + pos;
		char *e = buffer.ptr() + count;
		
		for (; b < e; b++) if (*b =='\n') break;
		
		if (size > 0) 
		{
			int n = b - (buffer.ptr() + pos);
			if (n > size) n = size;
			memcpy(s, buffer.ptr() + pos, n);
			size -= n;
			s += n;
		}
			
		pos = b - buffer.ptr();
			
		if (b < e) {
			pos++;
			break;
		}
	}
	*s = 0;
	
	return true;
}

class TextOutStream {
	int bufSize;
	carray<char> buffer;
	int pos;
public:
	TextOutStream(int _bSize = 1024):bufSize(_bSize>0 ? _bSize : 1024), pos(0){ buffer.alloc(bufSize); }
	void Flush(){ 	if (pos>0) { Write(buffer.ptr(), pos); pos = 0; } }
	void PutC(int c){ if (pos >= bufSize) Flush(); buffer[pos++] = c; }
	void Put(const char *s, int size);
	void Put(const char *s)	{ Put(s, strlen(s)); }
	~TextOutStream(){}
protected:
	virtual void Write(char *buf, int size) = 0; //can throw
	void Clear(){ pos = 0; }
};


void TextOutStream::Put(const char *s, int size)
{
	while (size > 0) 
	{
		if (pos >= bufSize) Flush();
		int n = bufSize - pos;
		if (n > size) n = size;
		memcpy(buffer.ptr()+pos, s, n);
		pos += n;
		size-=n;
		s+=n;
	}
}

class SysTextFileIn: public TextInStream {
	File f;
public:
	SysTextFileIn(int bSize = 4096):TextInStream(bSize){}
	void Open(const sys_char_t *fileName){ f.Open(fileName, FOPEN_READ); }
	void Close(){ f.Close(); }
	~SysTextFileIn(){}
	
protected:
	virtual int Read(char *buf, int size);
};

int SysTextFileIn::Read(char *buf, int size){ return f.Read(buf, size); }


class SysTextFileOut: public TextOutStream {
	File f;
public:
	SysTextFileOut(int bSize = 4096):TextOutStream(bSize) {}
	void Open(const sys_char_t *fileName){ Clear(); f.Open(fileName, FOPEN_WRITE | FOPEN_CREATE | FOPEN_TRUNC); }
	void Close(){ f.Close(); }
	~SysTextFileOut(){}
	
protected:
	virtual void Write(char *buf, int size);
};

void SysTextFileOut::Write(char *buf, int size){  f.Write(buf, size); }



static FSPath configDirPath(CS_UTF8, "???");

inline bool IsSpace(int c){ return c>0 && c<=32; }
inline bool IsLatinChar(int c){ return c>='a' && c<='z' || c>='A' && c<='Z'; }
inline bool IsDigit(int c){ return c>='0' && c<='9'; }




class IniHash {
	cstrhash< cstrhash< carray<char> > > hash;
	carray<char>* Find(const char *section, const char *var);
	carray<char>* Create(const char *section, const char *var);
	void Delete(const char *section, const char *var);
public:
	IniHash();
	void SetStrValue(const char *section, const char *var, const char *value);
	void SetIntValue(const char *section, const char *var, int value);
	void SetBoolValue(const char *section, const char *var, bool value);
	const char *GetStrValue(const char *section, const char *var, const char *def);
	int GetIntValue(const char *section, const char *var, int def);
	bool GetBoolValue(const char *section, const char *var, bool def);
	void Clear();
	void Load(const sys_char_t *fileName);
	void Save(const sys_char_t *fileName);
	~IniHash();
};

IniHash::IniHash(){}

carray<char>* IniHash::Find(const char *section, const char *var)
{
	cstrhash< carray<char> > *h = hash.exist(section);
	if (!h) return 0;
	return h->exist(var);
}

carray<char>* IniHash::Create(const char *section, const char *var){ return &(hash[section][var]); }
void IniHash::Delete(const char *section, const char *var){ hash[section].del(var, false); }
void IniHash::SetStrValue(const char *section, const char *var, const char *value){ if (!value) { Delete(section, var); return;}; carray<char>* p = Create(section, var); if (p) *p = new_char_str(value); }
void IniHash::SetIntValue(const char *section, const char *var, int value){ char buf[64]; int_to_char<int>(value, buf); SetStrValue(section, var, buf); }
void IniHash::SetBoolValue(const char *section, const char *var, bool value){ SetIntValue(section, var, value ? 1 : 0); }
const char *IniHash::GetStrValue(const char *section, const char *var, const char *def){ carray<char> *p =  Find(section, var); return (p && p->ptr()) ? p->ptr() : def; }
int IniHash::GetIntValue(const char *section, const char *var, int def){ carray<char> *p =  Find(section, var); return (p && p->ptr()) ? atoi(p->ptr()) : def; }
bool IniHash::GetBoolValue(const char *section, const char *var, bool def){ int n = GetIntValue(section, var, def ? 1 : 0); return n ? true : false; }

void IniHash::Clear(){ hash.clear(); }

void IniHash::Load(const sys_char_t *fileName)
{
//	Clear();
	SysTextFileIn in;
	try {
		in.Open(fileName);
	} catch (csyserr *ex) {
		if (SysErrorIsFileNotFound(ex->code))
		{
			ex->destroy();
			return;
		}
		throw;
	}
	
	char buf[4096];
	carray<char> section;
		
	while (in.GetLine(buf, sizeof(buf)))
	{

		char *s = buf;
		while (IsSpace(*s)) s++;
		if (!*s || *s == '#') continue;
		if (*s == '[')
		{
			s++;
			while (IsSpace(*s)) s++;
			char *t = s;
			while (*t && *t != ']') t++;
			if (*t != ']') continue;
			
			while ( t>s && IsSpace(*(t-1))) t--;
			*t = 0;
		
			section = new_char_str(s);
			
		} else {
			if (!section.ptr()) continue;

			char *t = s;
			while (*t && *t != '=') t++;
			if (*t != '=') continue;

			char *v = t + 1;
			while ( t>s && IsSpace(*(t-1))) t--;
			*t = 0;
			
			while (IsSpace(*v)) v++;
			t = v;
			while (*t) t++;
			while ( t>v && IsSpace(*(t-1))) t--;
			*t = 0;

			SetStrValue(section.ptr(), s, v);
		}
	}
	in.Close();
}

void IniHash::Save(const sys_char_t *fileName)
{
	SysTextFileOut out;
	out.Open(fileName);
	
	if (hash.count()>0) {
		carray<const char *> secList = hash.keys();
		sort2m<const char *>(secList.ptr(), hash.count(), strless< const char* >);
		for (int i = 0; i<hash.count(); i++)
		{
			out.Put("\n["); out.Put(secList[i]); out.Put("]\n");
			cstrhash< carray<char> > *h = hash.exist(secList[i]);
			if (!h) continue;
			
			carray<const char *> varList = h->keys();
			sort2m<const char *>(varList.ptr(), h->count(), strless< const char* >);
			for (int j = 0; j<h->count(); j++)
			{
				out.Put(varList[j]);
				out.PutC('=');
				carray<char> *p = h->exist(varList[j]);
				if (p && p->ptr()) out.Put(p->ptr());
				out.PutC('\n');
			}
		}
	}
	
	out.Flush();
	out.Close();
}

IniHash::~IniHash(){}

bool LoadStringList(const char *section, ccollect< carray<char> > &list)
{
	try {
		SysTextFileIn in;
				
		FSPath path = configDirPath;
		path.Push(CS_UTF8, carray_cat<char>(section, ".cfg").ptr());
		in.Open( (sys_char_t*)path.GetString(sys_charset_id) );
		
		char buf[4096];
		while (in.GetLine(buf, sizeof(buf))) 
		{
			char *s = buf;
			while (*s>0 && *s<=' ') s++;
			if (*s) list.append(new_char_str(s));
		}
	} catch (cexception *ex) {
		ex->destroy();
		return false;
	}
	return true;
}


void SaveStringList(const char *section, ccollect< carray<char> > &list)
{
	try {
		SysTextFileOut out;
				
		FSPath path = configDirPath;
		path.Push(CS_UTF8, carray_cat<char>(section, ".cfg").ptr());
		out.Open( (sys_char_t*)path.GetString(sys_charset_id) );
		
		for (int i = 0; i<list.count(); i++)
		{
			if (list[i].ptr() && list[i][0])
			{
				out.Put(list[i].ptr());
				out.PutC('\n');
			}
		}
		
		out.Flush();
		out.Close();
		
	} catch (cexception *ex) {
		ex->destroy();
		return ;
	}
}


#else
//старый клочек, надо перепроверить
static char *regapp = "Wal commander";
static char *regcomp = "Wal";

#define COMPANY regcomp
#define APPNAME regapp

static HKEY GetAppProfileKey()
{
	if (!regapp || !regcomp) return NULL;
	HKEY hsoft = NULL;
	HKEY hcomp = NULL;
	HKEY happ  = NULL;

	if (RegOpenKeyExA(HKEY_CURRENT_USER, "software", 0, KEY_WRITE|KEY_READ,
		&hsoft) == ERROR_SUCCESS) {
		DWORD dw;
		if (RegCreateKeyExA(hsoft, COMPANY, 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
			&hcomp, &dw) == ERROR_SUCCESS)
		{
			RegCreateKeyExA(hcomp, APPNAME, 0, REG_NONE,
				REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
				&happ, &dw);
		}
	}

	if (hsoft != NULL) RegCloseKey(hsoft);
	if (hcomp != NULL) RegCloseKey(hcomp);

	return happ;
}

static HKEY GetSection(const char *sectname)
{
	ASSERT(sectname && *sectname);
	HKEY happ = GetAppProfileKey();
	if (!happ) return NULL;
	DWORD dw;
	HKEY hsect;
	RegCreateKeyEx(happ, sectname, 0, REG_NONE,
				REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
				&hsect, &dw);
	RegCloseKey(happ);
	return hsect;
}

DWORD RegReadInt(const char *sect,const  char *what, DWORD def)
{
	HKEY hsect = GetSection(sect);
	if (!hsect) return def;
	DWORD dwValue;
	DWORD dwType;
	DWORD dwCount = sizeof(DWORD);
	LONG lResult = RegQueryValueEx(hsect, (LPTSTR)what, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	RegCloseKey(hsect);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_DWORD);
		ASSERT(dwCount == sizeof(dwValue));
		return (UINT)dwValue;
	}
	return def;
}

carray<char> RegReadString(char const *sect, const char *what, const char*def)
{
	HKEY hsect = GetSection(sect);
	if (!hsect) return def ? new_char_str(def) : carray<char>(0);

	carray<char> strValue;
	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hsect, (LPTSTR)what, NULL, &dwType,
		NULL, &dwCount);

	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		strValue.alloc(dwCount+1);
		strValue[dwCount] = 0;

		lResult = RegQueryValueEx(hsect, (LPTSTR)what, NULL, &dwType,
			(LPBYTE)strValue.ptr(), &dwCount);
	}

	RegCloseKey(hsect);

	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		return strValue;
	}

	return def ? new_char_str(def) : carray<char>(0);
}

int RegGetBinSize(const char *sect, const char *what)
{
	HKEY hsect = GetSection(sect);
	if (hsect == NULL) return -1;
	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hsect, (LPTSTR)what, NULL, &dwType,
		NULL, &dwCount);
	RegCloseKey(hsect);	
	if (lResult != ERROR_SUCCESS) return -1;
	return dwCount;

}	

bool RegReadBin(const char *sect, const char *what, const void *data, int size)
{
	HKEY hsect = GetSection(sect);
	if (hsect == NULL) return false;

	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hsect, (LPTSTR)what, NULL, &dwType,
		NULL, &dwCount);

	if (lResult != ERROR_SUCCESS || dwCount!=(DWORD)size) {
		RegCloseKey(hsect);	
		return false;
	}

	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_BINARY);
		lResult = RegQueryValueEx(hsect, (LPTSTR)what, NULL, &dwType,
			(LPBYTE)data, &dwCount);
	}

	RegCloseKey(hsect);
	return (lResult == ERROR_SUCCESS);
}

bool RegWriteInt(const char *sect, const char *what, DWORD data)
{
	HKEY hsect = GetSection(sect);
	if (hsect == NULL) return false;

	LONG lResult = RegSetValueEx(hsect, what, 0, REG_DWORD,
		(LPBYTE)&data, sizeof(data));
	RegCloseKey(hsect);
	return lResult == ERROR_SUCCESS;
}

bool RegWriteString(const char *sect, const char *what, const char *data)
{
	if (!data) return false;
	HKEY hsect = GetSection(sect);
	if (hsect == NULL) return false;
	LONG lResult = RegSetValueEx(hsect, what, 0, REG_SZ,
		(LPBYTE)data, strlen(data)+1);
	RegCloseKey(hsect);
	return lResult == ERROR_SUCCESS;
}

bool RegWriteBin(const char *sect, const char *what, const void *data, int size)
{
	HKEY hsect = GetSection(sect);
	if (hsect == NULL) 	return false;
	LONG lResult = RegSetValueEx(hsect, what, 0, REG_BINARY, 
		(LPBYTE)data, size);
	RegCloseKey(hsect);
	return lResult == ERROR_SUCCESS;
}

bool LoadStringList(const char *section, ccollect< carray<char> > &list)
{
	char name[64];
	list.clear();
	for (int i=1; ; i++)
	{
		snprintf(name, sizeof(name), "v%i", i);
		carray<char> s = RegReadString(section, name, "");
		if (!s.ptr() || !s[0]) break;
		list.append(s);
	}

	return true;
}

void SaveStringList(const char *section, ccollect< carray<char> > &list)
{
	int n = 1;
	char name[64];

	for (int i = 0; i<list.count(); i++)
	{
		if (list[i].ptr() && list[i][0])
		{
			snprintf(name, sizeof(name), "v%i", n);
			if (!RegWriteString(section, name, list[i].ptr()))
				break;
			n++;
		}
	}

	snprintf(name, sizeof(name), "v%i", n);
	RegWriteString(section, name, "");
}


#endif

ConfigStruct::ConfigStruct()
{
}


void ConfigStruct::MapInt(const char *section, const char *name, int *pInt, int def)
{
	Node node;
	node.type = MT_INT;
	node.section = section;
	node.name = name;
	node.ptr.pInt = pInt;
	node.def.defInt = def;
	mapList.append(node);
}

void ConfigStruct::MapBool(const char *section, const char *name, bool *pBool, bool def)
{
	Node node;
	node.type = MT_BOOL;
	node.section = section;
	node.name = name;
	node.ptr.pBool = pBool;
	node.def.defBool = def;
	mapList.append(node);
}

void ConfigStruct::MapStr(const char *section, const char *name, carray<char> *pStr, const char *def)
{
	Node node;
	node.type = MT_STR;
	node.section = section;
	node.name = name;
	node.ptr.pStr = pStr;
	node.def.defStr = def;
	mapList.append(node);
}


void ConfigStruct::Load()
{
#ifdef _WIN32
	
	for (int i = 0; i<mapList.count(); i++)
	{
		Node &node = mapList[i];
		if (node.type == MT_BOOL && node.ptr.pBool != 0)
			*node.ptr.pBool = RegReadInt(node.section, node.name, node.def.defBool)!=0;
		else
		if (node.type == MT_INT && node.ptr.pInt != 0)
			*node.ptr.pInt = RegReadInt(node.section, node.name, node.def.defInt);
		else
		if (node.type == MT_STR && node.ptr.pStr != 0)
			*node.ptr.pStr = RegReadString(node.section, node.name, node.def.defStr);
	}

#else 
	IniHash hash;
	FSPath path = configDirPath;
	path.Push(CS_UTF8, "config");
	
	hash.Load(DEFAULT_CONFIG_PATH);
	hash.Load((sys_char_t*)path.GetString(sys_charset_id));

	for (int i = 0; i<mapList.count(); i++)
	{
		Node &node = mapList[i];
		if (node.type == MT_BOOL && node.ptr.pBool != 0)
			*node.ptr.pBool = hash.GetBoolValue(node.section, node.name, node.def.defBool);
		else
		if (node.type == MT_INT && node.ptr.pInt != 0)
			*node.ptr.pInt = hash.GetIntValue(node.section, node.name, node.def.defInt);
		else
		if (node.type == MT_STR && node.ptr.pStr != 0)
		{
			const char *s = hash.GetStrValue(node.section, node.name, node.def.defStr);
			if (s) *node.ptr.pStr = new_char_str(s);
			else (*node.ptr.pStr) = 0;
		}

	}
#endif
//	if (editTabSize<=0 || editTabSize >64) editTabSize = 8;
}

void ConfigStruct::Save()
{
#ifdef _WIN32
	for (int i = 0; i<mapList.count(); i++)
	{
		Node &node = mapList[i];
		if (node.type == MT_BOOL && node.ptr.pBool != 0)
			RegWriteInt(node.section, node.name, *node.ptr.pBool);
		else
		if (node.type == MT_INT && node.ptr.pInt != 0)
			RegWriteInt(node.section, node.name, *node.ptr.pInt);
		else
		if (node.type == MT_STR && node.ptr.pStr != 0)
			RegWriteString(node.section, node.name, node.ptr.pStr->ptr());
	}
#else 
	IniHash hash;
	FSPath path = configDirPath;
	path.Push(CS_UTF8, "config");
	hash.Load((sys_char_t*)path.GetString(sys_charset_id));

	for (int i = 0; i<mapList.count(); i++)
	{
		Node &node = mapList[i];
		if (node.type == MT_BOOL && node.ptr.pBool != 0)
			hash.SetBoolValue(node.section, node.name, *node.ptr.pBool);
		else
		if (node.type == MT_INT && node.ptr.pInt != 0)
			hash.SetIntValue(node.section, node.name, *node.ptr.pInt);
		else
		if (node.type == MT_STR && node.ptr.pStr != 0)
			hash.SetStrValue(node.section, node.name, node.ptr.pStr->ptr());
	}
	
	hash.Save((sys_char_t*)path.GetString(sys_charset_id));
#endif
}


void InitConfigPath()
{
#ifdef _WIN32
#else 

	const sys_char_t *home = (sys_char_t*) getenv("HOME");
	
	if (home) 
	{
		FSPath path(sys_charset_id, home);
		path.Push(CS_UTF8, ".wcm");
		FSSys fs;
		FSStat st;
		int err;
		if (fs.Stat(path, &st, &err, 0))
		{
			if (fs.IsENOENT(err)) //директорий не существует
			{
				if (fs.MkDir(path, 0700, &err,  0)) 
				{
					fprintf(stderr, "can`t create config directory %s (%s)", path.GetUtf8(), fs.StrError(err).GetUtf8());
					return;
				}
				
			} else {
				fprintf(stderr, "can`t create config directory statuc %s (%s)", path.GetUtf8(), fs.StrError(err).GetUtf8());
				return;
			}
		} else 
		if (!st.IsDir())
		{
			fprintf(stderr, "err: '%s' is not directory", path.GetUtf8());
			return;
		}
		
		configDirPath = path;
	} else {
		fprintf(stderr, "err: HOME env value not found");
	}
#endif
}


