#ifndef COLOR_STYLE_H
#define COLOR_STYLE_H
#include <swl.h>

#define COLORSTYLE_VERSION "001"

extern char *currentColorStyleName;

//возвращает строку ошибки, если ошибок нет, то .ptr() == 0
wal::carray<char> SetColorStyle(const char *name);

void GetColorStyleList(wal::ccollect< wal::carray<char> > *list);

#endif
