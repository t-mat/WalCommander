#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H

#include <wal.h>

struct ConfigStruct {
	enum MapType { MT_BOOL, MT_INT, MT_STR };
	struct Node {
		MapType type;
		const char *section;
		const char *name;
		union {
			int *pInt;
			bool *pBool;
			wal::carray<char> *pStr;
		} ptr;
		union {
			int defInt;
			bool defBool;
			const char *defStr;
		} def;
	};

	wal::ccollect<Node, 64> mapList;
	void MapInt(const char *section, const char *name, int *pInt, int def);
	void MapBool(const char *section, const char *name, bool *pInt, bool def);
	void MapStr(const char *section, const char *name, wal::carray<char> *pStr, const char *def = 0);
	ConfigStruct();
	void Load();
	void Save();
};

void InitConfigPath();

bool LoadStringList(const char *section, wal::ccollect< wal::carray<char> > &list);
void SaveStringList(const char *section, wal::ccollect< wal::carray<char> > &list);

#endif

