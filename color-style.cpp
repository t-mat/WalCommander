#include "color-style.h"
#include "vfs.h"
#include "string-util.h"
#include "vfs-uri.h"
#include "globals.h"

//надо нормально разукрасить

using namespace wal;

static char uiDefaultWcmRules[] = 
//"* {mode3d:0} "
"ComboBox, ComboBox EditLine {color:0;  background:0xFFFFFF }"
"ComboBox EditLine:parent-focus {color: 0; background: 0x909000; mark-color: 0xFFFFFF; mark-background : 0x800000; frame-color: 0xD8E9EC }" //{color:0xFFFFFF;  background:0x800000 }"
"ComboBox *@item {color: 0; background: 0xFFFFFF; }"
"ComboBox *@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

"NCEditLine, NCEditLine EditLine:!enabled {color:0;  background:0xD8E9EC; }"
"NCEditLine EditLine:parent-focus:enabled {color: 0; background: 0x909000; mark-color: 0xFFFFFF; mark-background : 0x800000; frame-color: 0xD8E9EC }"
"NCEditLine *@item {color: 0; background: 0xFFFFFF; }"
"NCEditLine *@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

"#variable { color : 0x800000 }"
"#value { color : 0x9000 }"

"Progress { color:0xA00000; frame-color: 0x808080; background: 0xD8E9EC }"

"* {color: 0; background: 0xD8E9EC; focus-frame-color : 0x00A000; button-color: 0xD8E9EC; hotkey-color: 0xD0 }"
"*@item:odd		{color: 0; background: 0xE0FFE0; }"
"*@item:current-item:focus	{color: 0xFFFFFF; background: 0x800000; }"
"*@item:current-item	{color: 0xFFFFFF; background: 0x808080; }"
"*@item			{color: 0; background: 0xFFFFFF; }"

"ScrollBar { button-color: 0xD8E9EC;  background: 0xC8D9DC; }"
"EditLine:!enabled { background: 0xD8E9EC; }"
"EditLine:focus:enabled {color: 0; background: 0x909000; mark-color: 0xFFFFFF; mark-background : 0x800000; frame-color: 0xD8E9EC }"
"EditLine {color: 0; background: 0x909000; frame-color: 0xD8E9EC; mark-color: 0xFFFFFF; mark-background : 0x808080; }"

//"Panel { color: 0xFFFF80; background: 0x800000; fssize-color: 0x00FF00; }"
"Panel { color: 0xFFFF80; background: 0x800000; fssize-color: 0x00FF00; }"
"Panel@item:exe { color: 0x00FF00 }"
"Panel@item:dir { color: 0xFFFFFF }"
"Panel@item:hidden { color: 0xFF8080 }"
"Panel@item:hidden:selected-panel:current-item { color: 0x804040 }"
"Panel@item:bad { color: 0xA0 }"
"Panel@item:selected { color: 0x00FFFF }"
"Panel@item:selected-panel:current-item { background: 0x808000 }" //background: 0xB0B000 }"
"Panel@item:selected-panel:current-item:oper-state { background: 0xFF }"
//"Panel ScrollBar, Viewer ScrollBar, Editor ScrollBar {  background: 0xA0A000; button-color: 0xFFFF00 }"
"Panel ScrollBar, Viewer ScrollBar, Editor ScrollBar {  background: 0x808000; button-color: 0xA0A000 }"
"Panel { border-color1: 0x000000; border-color2: 0xFFFFFF; border-color3: 0xFFFFFF; border-color4: 0x000000;"
	" vline-color1: 0xFFFFFF; vline-color2:0x707070; vline-color3:0x700000;"
	" line-color:0xFFFFFF; summary-color: 0xFFFFFF; }"
"Panel@footer { color: 0xFFFFFF; background: 0x800000; }"
"Panel { summary-color: 0xFFFF00; }"
"Panel:have-selected { summary-color: 0xFFFF; }"
"Panel:oper-state { summary-color: 0xFF; }"

"ButtonWin { color: 0xFFFFFF; background: 0; }"
"ButtonWin Button { color: 0; background: 0xB0B000; }"
//
"#command-line EditLine {color: 0xFFFFFF; background: 0; mark-color: 0xFFFFFF; mark-background : 0x800000; }"
"#command-line * {color: 0xFFFFFF; background: 0x808000; frame-color: 0xFFFFFF; }" 
"#command-line * @item:current-item {color: 0xFFFFFF; background: 0; frame-color: 0;  }"
"#command-line {color: 0xFFFFFF; background: 0x808000; }"

"ToolTip { color: 0; background: 0x80FFFF; }"
"ToolBar { color: 0; background: 0xB0B000; }"

"MenuBar, PopupMenu { hotkey-color: 0xFFFF }"

"MenuBar { color: 0; background: 0xB0B000; current-item-frame-color:0x00FF00; }"
"MenuBar:current-item { color: 0xFFFFFF; background: 0;  } "
"PopupMenu@item {color: 0xFFFFFF; background: 0xA0A000; pointer-color:0; line:0; } "
"PopupMenu@item:current-item { background: 0x404000; } "
"PopupMenu { background: 0xC0C000; frame-color: 0xFFFFFF; } "

"#drive-dlg { background: 0xB0B000; }"
"#drive-dlg * { background: 0xB0B000; color: 0xFFFFFF; }"
"#drive-dlg * @item { color: 0xFFFFFF; background: 0xB0B000; comment-color: 0xFFFF00; first-char-color:0xFFFF }"
"#drive-dlg * @item:current-item { color: 0xFFFFFF;  background: 0; comment-color: 0xFFFF00; first-char-color:0xFFFF }"

"#messagebox-red { color: 0xFFFFFF; background: 0xFF; }"
"#messagebox-red Static { color: 0xFFFFFF; background: 0xFF; }"
"HelpWin@style-head { color: 0xFFFFFF; }"
"HelpWin@style-def  { color: 0xFFFFFF; }"
"HelpWin@style-red  { color: 0xFF; }"
"HelpWin@style-bold { color: 0xFFFF; }"
"HelpWin { background: 0x808000; }"

"Viewer { background: 0x800000; color:0xFFFFFF; ctrl-color:0xFF0000; "
"	mark-color:	0x0; mark-background: 0xFFFFFF; line-color:0x00FFFF; "
"	hid: 0xB00000; load-background:	0xD00000; load-color: 0xFF;}"
"Editor { background: 0x800000; color:0xFFFFFF; mark-background: 0xFFFFFF; mark-color: 0; cursor-color: 0xFFFF00; "
"	DEF:0xFFFFFF; KEYWORD:0x00FFFF; COMMENT:0xFF8080; STRING:0xFFFF00; PRE:0x00FF00; NUM:0xFFFF80; OPER:0x20FFFF; ATTN:0x0000FF; }"
"StringWin { color: 0xFFFF00; background: 0; }"
"EditorHeadWin, ViewerHeadWin { color: 0; background: 0xB0B000; prefix-color:0xFFFFFF; cs-color:0xFFFFFF;}"

"TabDialog { background: 0x606060; color:0xFFFFFF; }"
"TabDialog * { background: 0x606060; color:0xFFFFFF; button-color: 0x808080; }"
"TabDialog EditLine { background: 0xD0D0D0; color:0; frame-color: 0x606060; }"
"TabDialog *@item:dir {color: 0xFFFF00; background: 0; }"
"TabDialog *@item:current-item	{background: 0x808080; }"
"TabDialog *@item {color: 0xFFFFFF; background: 0; }"

"Shortcuts VListWin { background: 0xB0B000; first-char-color:0xFFFF; }"
"Shortcuts VListWin@item {color: 0; background: 0xB0B000; }"
"Shortcuts VListWin@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

;


//!*************************************************************************************************


static char uiBlackWcmRules[] = 
"ComboBox, ComboBox EditLine {color:0;  background:0xFFFFFF }"
"ComboBox EditLine:parent-focus {color:0xFFFFFF;  background:0x800000 }"
"ComboBox *@item {color: 0; background: 0xFFFFFF; }"
"ComboBox *@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

//"NCEditLine, NCEditLine EditLine:!enabled {color:0;  background:0xD8E9EC; }"
"NCEditLine * {frame-color: 0xFFFFFF } " //EditLine:parent-focus:enabled {color: 0; background: 0x909000; mark-color: 0xFFFFFF; mark-background : 0x800000; frame-color: 0xD8E9EC }"
//"NCEditLine *@item {color: 0; background: 0xFFFFFF; }"
//"NCEditLine *@item:current-item {color: 0xFFFFFF; background: 0x800000; }"


"* {color: 0; background: 0xA0B0B0; focus-frame-color : 0x00A000; button-color: 0xD8E9EC; hotkey-color: 0xFFFF  }"
"*@item:odd		{color: 0xFFFFFF; background: 0x101010; }"
"*@item:current-item:focus	{color: 0xFFFFFF; background: 0x800000; }"
"*@item:current-item	{color: 0xFFFFFF; background: 0x505050; }"
"*@item			{color: 0xFFFFFF; background: 0; }"

"Progress { color:0xA00000; frame-color: 0x808080; background: 0xD8E9EC }"

"ScrollBar { button-color: 0xD8E9EC;  }"
"EditLine:!enabled { background: 0xD8E9EC; }"
"EditLine:focus:enabled {color: 0; background: 0x909000; mark-color: 0xFFFFFF; mark-background : 0x800000; frame-color: 0xA0B0B0 }"
"EditLine {color: 0; background: 0x909000; frame-color: 0xA0B0B0; mark-color: 0xFFFFFF; mark-background : 0x808080; }"

"Panel { color: 0xA0A0A0; background: 0x000000; fssize-color: 0x00FF00; }"
"Panel@item:exe { color: 0x80FF80 }"
"Panel@item:dir { color: 0xFFFFFF }"
"Panel@item:hidden { color: 0xB06060 }"
"Panel@item:hidden:selected-panel:current-item { color: 0x804040 }"
"Panel@item:bad { color: 0xA0 }"
"Panel@item:selected { color: 0x00FFFF }"
"Panel@item:selected-panel:current-item { background: 0xA0A000 }"

"Panel@item:selected-panel:current-item:!exe:!dir:!hidden:!bad:!selected { color:0 }"

"Panel@item:selected-panel:current-item:oper-state { background: 0xFF }"
"Panel ScrollBar, Viewer ScrollBar, Editor ScrollBar {  background: 0x808080; button-color: 0xB0B000 }"
"Panel { border-color1: 0x000000; border-color2: 0xFFFFFF; border-color3: 0xFFFFFF; border-color4: 0x000000;"
	" vline-color1: 0xFFFFFF; vline-color2:0x707070; vline-color3:0x700000;"
	" line-color:0xFFFFFF; summary-color: 0xFFFFFF; }"
"Panel@footer { color: 0xFFFFFF; background: 0; }"
"Panel@header:selected-panel	{ color: 0xFFFFFF; }"
"Panel { summary-color: 0xFFFF00; }"
"Panel:have-selected { summary-color: 0xFFFF; }"
"Panel:oper-state { summary-color: 0xFF; }"

"ButtonWin { color: 0xFFFFFF; background: 0; }"
"ButtonWin Button { color: 0; background: 0xA0A000; }"

//"EditLine#command-line {color: 0xFFFFFF; background: 0; mark-color: 0xFFFFFF; mark-background : 0x800000; }"
"#command-line EditLine {color: 0xFFFFFF; background: 0; mark-color: 0xFFFFFF; mark-background : 0x800000; }"
"#command-line * {color: 0xFFFFFF; background: 0x808000; frame-color: 0xFFFFFF; }" 
"#command-line * @item:current-item {color: 0xFFFFFF; background: 0; frame-color: 0;  }"
"#command-line {color: 0xFFFFFF; background: 0x808000; }"

"ToolTip { color: 0; background: 0x80FFFF; }"
"ToolBar { color: 0; background: 0xA0A000; }"
"MenuBar { color: 0; background: 0xA0A000; current-item-frame-color:0x00FF00;}"
"MenuBar:current-item { color: 0xFFFFFF; background: 0; } "
"PopupMenu@item {color: 0xFFFFFF; background: 0xA0A000; pointer-color:0; line:0; } "
"PopupMenu@item:current-item { background: 0x404000; } "
"PopupMenu { background: 0xC0C000; frame-color: 0xFFFFFF; } "

"#drive-dlg { background: 0xA0A000; }"
"#drive-dlg * { background: 0xA0A000; color: 0xFFFFFF; }"
"#drive-dlg * @item { color: 0xFFFFFF; background: 0xA0A000; comment-color: 0xFFFF00; first-char-color:0xFFFF }"
"#drive-dlg * @item:current-item { color: 0xFFFFFF;  background: 0; comment-color: 0xFFFF00; first-char-color:0xFFFF }"

"#messagebox-red { color: 0xFFFFFF; background: 0xFF; }"
"#messagebox-red Static { color: 0xFFFFFF; background: 0xFF; }"
"HelpWin@style-head { color: 0xFFFFFF; }"
"HelpWin@style-def  { color: 0xFFFFFF; }"
"HelpWin@style-red  { color: 0xFF; }"
"HelpWin@style-bold { color: 0xFFFF; }"
"HelpWin { background: 0; }"

"Viewer { background: 0x000000; color:0xB0B0B0; ctrl-color:0x800000; "
"	mark-color:	0x0; mark-background: 0xFFFFFF; line-color:0x008080; "
"	hid: 0x600000; load-background:	0x300000; load-color: 0x80;}"
"Editor { background: 0x000000; color:0xFFFFFF; mark-background: 0xFFFFFF; mark-color: 0; cursor-color: 0xFFFF00; "
"	DEF:0xE0E0E0; KEYWORD:0x6080F0; COMMENT:0xA08080; STRING:0xFFFF00; PRE:0x00D000; NUM:0xFFFF80; OPER:0x20D0D0; ATTN:0x0000FF; }"
"StringWin { color: 0xFFFF00; background: 0; }"
"EditorHeadWin, ViewerHeadWin { color: 0; background: 0xA0A000; prefix-color:0xFFFFFF; cs-color:0xFFFFFF;}"

"TabDialog { background: 0x606060; color:0xFFFFFF; }"
"TabDialog * { background: 0x606060; color:0xFFFFFF; button-color: 0x808080; }"
"TabDialog EditLine { background: 0xD0D0D0; color:0; frame-color: 0x606060; }"
"TabDialog *@item:dir {color: 0xFFFF00; background: 0; }"
"TabDialog *@item:current-item	{background: 0x808080; }"
"TabDialog *@item {color: 0xFFFFFF; background: 0; }"

"Shortcuts VListWin { background: 0; first-char-color:0xFFFF; }"
"Shortcuts VListWin@item {color: 0xFFFFFF; background: 0; }"
"Shortcuts VListWin@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

;

//!*************************************************************************************************

static char uiWhiteWcmRules[] = 
"ComboBox, ComboBox EditLine {color:0;  background:0xFFFFFF }"
"ComboBox EditLine:parent-focus {color:0xFFFFFF;  background:0x800000 }"
"ComboBox *@item {color: 0; background: 0xFFFFFF; }"
"ComboBox *@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

"* {color: 0; background: 0xD8E9EC; focus-frame-color : 0x00A000; button-color: 0xD8E9EC;  }"
"*@item:odd		{color: 0; background: 0xE0FFE0; }"
"*@item:current-item:focus	{color: 0xFFFFFF; background: 0x800000; }"
"*@item:current-item	{color: 0xFFFFFF; background: 0x808080; }"
"*@item			{color: 0; background: 0xFFFFFF; }"

"Progress { color:0xA00000; frame-color: 0x808080; background: 0xD8E9EC }"

"ScrollBar { button-color: 0xD8E9EC;  }"
"EditLine:!enabled { background: 0xD8E9EC; }"
"EditLine:focus:enabled {color: 0; background: 0xFFFFFF; mark-color: 0xFFFFFF; mark-background : 0x800000; frame-color: 0xD8E9EC }"
"EditLine {color: 0; background: 0xFFFFFF; frame-color: 0xD8E9EC; mark-color: 0xFFFFFF; mark-background : 0x808080; }"

"Panel { color: 0x000000; background: 0xF8F8F8; fssize-color: 0x008000;}"
"Panel@item:odd { background: 0xE0E0E0 }"
"Panel@item:exe { color: 0x306010 }"
"Panel@item:dir { color: 0 }"
"Panel@item:hidden { color: 0xA08080 }"
"Panel@item:hidden:selected-panel:current-item { color: 0x808080 }"
"Panel@item:bad { color: 0xA0 }"
"Panel@item:selected, Panel@item:selected:selected-panel:current-item { color: 0x0000FF }"
"Panel@item:selected-panel:current-item { background: 0x800000; color:0xFFFFFF; }" // mode3d:1; }"
"Panel@item:selected-panel:current-item:oper-state { background: 0xFF }"
"Panel ScrollBar, Viewer ScrollBar, Editor ScrollBar {  background: 0xE0E0E0; button-color: 0xFFD080 }"
"Panel { border-color1: 0x800000; border-color2: 0xD8E9EC; border-color3: 0xD8E9EC; border-color4: 0xD8E9EC;"
	" vline-color1: 0xE0E0E0; vline-color2:0x800000; vline-color3:0xE0E0E0;"
	" line-color:0x800000; summary-color: 0xFFFFFF; }"
"Panel@footer { color: 0; background: 0xD8E9EC; }"
"Panel@header:selected-panel	{ color: 0; background: 0xFFD0A0; }"
"Panel@header 			{ color: 0; background: 0xD8E9EC; }"
"Panel { summary-color: 0x800000; }"
"Panel:have-selected { summary-color: 0xFF; }"
"Panel:oper-state { summary-color: 0x8080; }"

"ButtonWin { color: 0; background: 0xD8E9EC; }"
"ButtonWin Button { color: 0; background: 0xD8E9EC; }"
"EditLine#command-line {color: 0; background: 0xFFFFFF; mark-color: 0xFFFFFF; mark-background : 0x800000; }"
"ToolTip { color: 0; background: 0x80FFFF; }"
"ToolBar { color: 0; background: 0xD8E9EC; }"
"MenuBar { color: 0; background: 0xD8E9EC; current-item-frame-color:0x00A000; }"
"MenuBar:current-item { color: 0xFFFFFF; background: 0;  } "

"PopupMenu@item {color: 0; background: 0xFFFFFF; pointer-color:0; line:0; } "
"PopupMenu@item:current-item { background: 0xB0C0C0; } "
"PopupMenu { background: 0xF0F0F0; frame-color: 0x808080; } "

"#drive-dlg { background: 0xD8E9EC; }"
"#drive-dlg * { background: 0xD8E9EC; color: 0; first-char-color:0xFF; }"
"#drive-dlg * @item { color: 0; background: 0xD8E9EC; comment-color: 0x808000;  }"
"#drive-dlg * @item:current-item { color: 0xFFFFFF;  background: 0; comment-color: 0xFFFF00; }"

"#messagebox-red { color: 0xFFFFFF; background: 0xFF; }"
"#messagebox-red Static { color: 0xFFFFFF; background: 0xFF; }"
"HelpWin@style-head { color: 0; }"
"HelpWin@style-def  { color: 0; }"
"HelpWin@style-red  { color: 0xFF; }"
"HelpWin@style-bold { color: 0x8080; }"
"HelpWin { background:  0xF8F8F8; }"

"Viewer { background:0xF8F8F8 ; color:0x000000; ctrl-color:0xFF0000; "
"	mark-color:	0x0; mark-background: 0xFFFFFF; line-color:0x800000; "
"	hid: 0xD0D0D0; load-background:	0xD00000; load-color: 0xFF;}"
"Editor { background:0xF8F8F8 ; color:0x000000; mark-background: 0x800000; mark-color: 0xFFFFFF; cursor-color: 0xFF0000; "
"	DEF:0; KEYWORD:0xFF0000; COMMENT:0xA0A0A0; STRING:0x808000; PRE:0x008000; NUM:0x000080; OPER:0x000080; ATTN:0x0000FF; }"
"StringWin { color: 0; background: 0xD8E9EC; }"
"EditorHeadWin, ViewerHeadWin { color: 0; background: 0xD8E9EC; prefix-color:0x808000; cs-color:0x0000FF;}"

"TabDialog { background: 0x606060; color:0xFFFFFF; }"
"TabDialog * { background: 0x606060; color:0xFFFFFF; button-color: 0x808080; }"
"TabDialog EditLine { background: 0xD0D0D0; color:0; frame-color: 0x606060; }"
"TabDialog *@item:dir {color: 0xFFFF00; background: 0; }"
"TabDialog *@item:current-item	{background: 0x808080; }"
"TabDialog *@item {color: 0xFFFFFF; background: 0; }"

"Shortcuts VListWin { background: 0xFFFFFF; first-char-color:0xFF; }"
"Shortcuts VListWin@item {color: 0; background: 0xFFFFFF; }"
"Shortcuts VListWin@item:current-item {color: 0xFFFFFF; background: 0x800000; }"

;


char *currentColorStyleName = 0;
static carray<char> styleNameBuf;

struct StyleNode {
	carray<char> name; //utf8
	char *style; 
	carray<sys_char_t> path;
	StyleNode():style(0){}
	StyleNode(const char *n, char *s, const sys_char_t *p);
};

StyleNode::StyleNode(const char *n, char *s, const sys_char_t *p)
:	name( new_char_str(n) ), style(s)
{
	if (p) path = new_sys_str(p);
}

struct StyleList {
	ccollect<StyleNode> list;

	//ищет по названию, если не находит, то добавляет
	void Add(const char *name, const sys_char_t *path);
	StyleList();
};

#ifdef _WIN32
#include "w32util.h"
#endif

StyleList::StyleList()
{
	list.append( StyleNode("Default", uiDefaultWcmRules, 0) );
	list.append( StyleNode("Black", uiBlackWcmRules, 0) );
	list.append( StyleNode("White", uiWhiteWcmRules, 0) );

	carray<unicode_t> uri;

#ifdef _WIN32
	uri = carray_cat<unicode_t> ( sys_to_unicode_array( GetAppPath().ptr() ).ptr(), utf8_to_unicode("style").ptr());
#else
	uri = utf8_to_unicode(UNIX_CONFIG_DIR_PATH "/style");
#endif
	FSPath path;
	FSPtr fs = ParzeURI(uri.ptr(), path, 0, 0);
	FSList dirList;

	if (fs.IsNull() || fs->ReadDir(&dirList, path, 0, 0) )
		return;

	int count = dirList.Count();
	carray<FSNode*> d = dirList.GetArray();
	dirList.SortByName(d.ptr(), count, true, true);

	for (int i = 0; i < count; i++)
	{
		const char *s = d[i]->Name().GetUtf8();
		const char *prev = 0;
		const char *last = 0;
		const char *t = s;
		for (; *t; t++)
			if (*t == '.') {
				prev = last;
				last = t;
			}
		if (!last || !prev) continue;
		
		if (strcmp(last + 1, "style")) continue;

		static char version[] = COLORSTYLE_VERSION ".";

		if (strncmp(version, prev + 1, strlen(version))) continue;
		int nameLen = prev - s;
		if (nameLen <= 0) continue;

		carray<char> name(nameLen + 1);
		memcpy(name.ptr(), s, nameLen);
		name[nameLen] = 0;
		
		path.PushStr(d[i]->Name());
		Add(name.ptr(), unicode_to_sys_array( fs->Uri(path).GetUnicode()).ptr() );
		path.Pop();
	}
}

void StyleList::Add(const char *name, const sys_char_t *path)
{
	for (int i = 0; i<list.count(); i++)
		if ( !strcmp(name, list[i].name.ptr()) ) {
			list[i].path = new_sys_str(path);
			return;
		}

	list.append( StyleNode(name, 0, path) );
}

//возвращает строку ошибки, если ошибок нет, то .ptr() == 0
wal::carray<char> SetColorStyle(const char *name)
{
	if (!name) name = "Default";
	carray<char> err;
	StyleList slist;

	for (int i = 0; i<slist.list.count(); i++)
		if (!strcmp(slist.list[i].name.ptr(), name)) 
		{
			try {
				if (slist.list[i].path.ptr())
					UiReadFile(slist.list[i].path.ptr());
				else if (slist.list[i].style)
					UiReadMem(slist.list[i].style);
				else 
					return err; //BOTVA

				styleNameBuf = new_char_str(name);
				currentColorStyleName = styleNameBuf.ptr();
			} catch (cexception *ex) {
				err = new_char_str(ex->message());
				ex->destroy();
			}
			return err;		
		};
	return err;
}

void GetColorStyleList(ccollect< carray<char> > *list)
{
	StyleList slist;
	for (int i = 0; i<slist.list.count(); i++)
		list->append( new_char_str( slist.list[i].name.ptr() ) );
}
