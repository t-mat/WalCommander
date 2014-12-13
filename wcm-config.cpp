#include <wal.h>
#include "wcm-config.h"
#include "color-style.h"
#include "string-util.h"
#include "vfs.h"
#include "fontdlg.h"
#include "ncfonts.h"
#include "ltext.h"
#include "globals.h"

#ifdef _WIN32
#include "w32util.h"
#endif

WcmConfig wcmConfig;
const char *sectionSystem = "system";
const char *sectionPanel = "panel";
const char *sectionEditor = "editor";
const char *sectionViewer = "viewer";
const char *sectionTerminal = "terminal";
const char *sectionFonts = "fonts";
const char *sectionStyle = "style";

WcmConfig::WcmConfig()
:	systemAskOpenExec(true),
	systemEscPanel(false),
	systemLang(new_char_str("+")),
	systemSaveHistory(true),
	systemAutoComplete(true),
	showToolBar(true),
	showButtonBar(true),

	panelShowHiddenFiles(true),
	panelShowIcons(true),
	panelCaseSensitive(
#ifdef _WIN32		
	false
#else
	true
#endif		
	),
	panelModeLeft(0),
	panelModeRight(0),
		
	editSavePos(true),
	editAutoIdent(true),
	editTabSize(8),
	//editColorMode(0),
	editShl(true),
	
	terminalBackspaceKey(0)

	//viewColorMode(0)
{
#ifndef _WIN32
	MapBool(sectionSystem, "ask_open_exec", &systemAskOpenExec, systemAskOpenExec);
#endif
	MapStr(sectionStyle,  "style", &style);

	MapBool(sectionSystem, "esc_panel", &systemEscPanel, systemEscPanel);
	MapStr(sectionSystem,  "lang", &systemLang);
	MapBool(sectionSystem, "save_history", &systemSaveHistory, systemSaveHistory);
	MapBool(sectionSystem, "save_auto_complete", &systemAutoComplete, systemAutoComplete);
	
	MapBool(sectionSystem, "show_toolbar", &showToolBar, showToolBar);
	MapBool(sectionSystem, "show_buttonbar", &showButtonBar, showButtonBar);

	MapBool(sectionPanel, "show_hidden_files",	&panelShowHiddenFiles, panelShowHiddenFiles);
	MapBool(sectionPanel, "show_icons",		&panelShowIcons, panelShowIcons);
	MapBool(sectionPanel, "case_sensitive_sort",	&panelCaseSensitive, panelCaseSensitive);
	//MapInt(sectionPanel,  "color_mode",		&panelColorMode, panelColorMode);
	MapInt(sectionPanel,  "mode_left",		&panelModeLeft, panelModeLeft);
	MapInt(sectionPanel,  "mode_right",		&panelModeRight, panelModeRight);

	MapBool(sectionEditor, "save_file_position",		&editSavePos, editSavePos);
	MapBool(sectionEditor, "auto_ident",	&editAutoIdent, editAutoIdent);
//	MapInt(sectionEditor,  "color_mode",	&editColorMode, editColorMode);
	MapInt(sectionEditor, "tab_size",	&editTabSize, editTabSize);
	MapBool(sectionEditor, "highlighting",	&editShl, editShl);
	
	MapInt(sectionTerminal, "backspace_key",	&terminalBackspaceKey, terminalBackspaceKey);
	
	MapStr(sectionFonts, "panel_font",	&panelFontUri);
	MapStr(sectionFonts, "viewer_font",	&viewerFontUri);
	MapStr(sectionFonts, "editor_font",	&editorFontUri);
	MapStr(sectionFonts, "dialog_font",	&dialogFontUri);
	MapStr(sectionFonts, "terminal_font",	&terminalFontUri);
	MapStr(sectionFonts, "helptext_font",	&helpTextFontUri);
	MapStr(sectionFonts, "helpbold_font",	&helpBoldFontUri);
	MapStr(sectionFonts, "helphead_font",	&helpHeadFontUri);

//	MapInt(sectionViewer,  "color_mode",	&viewColorMode, viewColorMode);
}

void WcmConfig::ImpCurrentFonts()
{
	panelFontUri = new_char_str(panelFont.ptr() ? panelFont->uri() : "");
	viewerFontUri = new_char_str(viewerFont.ptr() ? viewerFont->uri() : "");
	editorFontUri = new_char_str(editorFont.ptr() ? editorFont->uri() : "");
	dialogFontUri  = new_char_str(dialogFont.ptr() ? dialogFont->uri() : "");
	terminalFontUri  = new_char_str(terminalFont.ptr() ? terminalFont->uri() : "");
	helpTextFontUri  = new_char_str(helpTextFont.ptr() ? helpTextFont->uri() : "");
	helpBoldFontUri  = new_char_str(helpBoldFont.ptr() ? helpBoldFont->uri() : "");
	helpHeadFontUri  = new_char_str(helpHeadFont.ptr() ? helpHeadFont->uri() : "");
}

void WcmConfig::Load()
{
	ConfigStruct::Load();
	if (editTabSize<=0 || editTabSize >64) editTabSize = 8;
}



////////////////////////////////  PanelOptDlg

class PanelOptDialog: public NCVertDialog {
	Layout iL;
public:	
	SButton  showHiddenButton;
	SButton  showIconsButton;
	SButton  caseSensitive;
	
	PanelOptDialog(NCDialogParent *parent);
	virtual ~PanelOptDialog();
};

PanelOptDialog::~PanelOptDialog(){}

PanelOptDialog::PanelOptDialog(NCDialogParent *parent)
:	NCVertDialog(::createDialogAsChild, 0, parent, utf8_to_unicode( _LT( "Panel settings") ).ptr(), bListOkCancel),
	iL(16, 3),
	showHiddenButton(0, this, utf8_to_unicode( _LT("Show &hidden files") ).ptr(), 0, wcmConfig.panelShowHiddenFiles),
	showIconsButton(0, this, utf8_to_unicode( _LT("Show &icons") ).ptr(), 0, wcmConfig.panelShowIcons),
	caseSensitive(0, this, utf8_to_unicode( _LT("&Case sensitive sort") ).ptr(), 0, wcmConfig.panelCaseSensitive)
{
	iL.AddWin(&showHiddenButton,	0, 0); showHiddenButton.Enable(); showHiddenButton.SetFocus(); showHiddenButton.Show(); 
	iL.AddWin(&showIconsButton,	1, 0); showIconsButton.Enable(); showIconsButton.Show(); 
	iL.AddWin(&caseSensitive,	2, 0); caseSensitive.Enable(); caseSensitive.Show(); 
	AddLayout(&iL);
	SetEnterCmd(CMD_OK);

	showHiddenButton.SetFocus();
	
	order.append(&showHiddenButton);
	order.append(&showIconsButton);
	order.append(&caseSensitive);
	SetPosition();
}

bool DoPanelConfigDialog(NCDialogParent *parent)
{
	PanelOptDialog dlg(parent);

	if (dlg.DoModal() == CMD_OK)
	{
		wcmConfig.panelShowHiddenFiles = dlg.showHiddenButton.IsSet(); 
		wcmConfig.panelShowIcons = dlg.showIconsButton.IsSet(); 
		wcmConfig.panelCaseSensitive = dlg.caseSensitive.IsSet(); 
		return true;
	}

	return false;
}


////////////////////////////////  EditOptDlg


class EditOptDialog: public NCVertDialog {
	Layout iL;
public:	
	SButton  saveFilePosButton;
	SButton  autoIdentButton;
	SButton  shlButton;

	HKStaticLine tabText; 
	EditLine tabEdit;
	
	EditOptDialog(NCDialogParent *parent);
//	virtual bool EventChildKey(Win* child, cevent_key* pEvent);
//	virtual bool Command(int id, int subId, Win *win, void *data);
	virtual ~EditOptDialog();
};

EditOptDialog::~EditOptDialog(){}

EditOptDialog::EditOptDialog(NCDialogParent *parent)
:	NCVertDialog(::createDialogAsChild, 0, parent, utf8_to_unicode( _LT("Editor") ).ptr(), bListOkCancel),
	iL(16, 2),

	saveFilePosButton(0, this, utf8_to_unicode( _LT("Save &file position") ).ptr(), 0, wcmConfig.editSavePos),
	autoIdentButton(0, this, utf8_to_unicode( _LT("&Auto indent") ).ptr(), 0, wcmConfig.editAutoIdent),
	shlButton(0, this, utf8_to_unicode( _LT("&Syntax highlighting") ).ptr(), 0, wcmConfig.editShl),
	tabText(0, this, utf8_to_unicode( _LT("&Tab size:") ).ptr()), 
	tabEdit(0, this, 0, 0, 16)
{
	char buf[0x100];
	snprintf(buf, sizeof(buf)-1, "%i", wcmConfig.editTabSize);
	tabEdit.SetText(utf8_to_unicode(buf).ptr(), true);

	iL.AddWin(&saveFilePosButton,	0, 0, 0, 1); saveFilePosButton.Enable(); saveFilePosButton.Show(); 
	iL.AddWin(&autoIdentButton,	1, 0, 1, 1); autoIdentButton.Enable(); autoIdentButton.Show(); 
	iL.AddWin(&shlButton,		2, 0, 2, 1); shlButton.Enable(); shlButton.Show(); 
	
	iL.AddWin(&tabText, 		3, 0, 3, 0); tabText.Enable(); tabText.Show();
	tabText.SetControlWin(&tabEdit);
	iL.AddWin(&tabEdit, 		4, 1, 4, 1); tabEdit.Enable(); tabEdit.Show(); 
	AddLayout(&iL);
	SetEnterCmd(CMD_OK);
	
	saveFilePosButton.SetFocus();
	
	order.append(&saveFilePosButton);
	order.append(&autoIdentButton);
	order.append(&shlButton);
	order.append(&tabEdit);
	SetPosition();
}

bool DoEditConfigDialog(NCDialogParent *parent)
{
	EditOptDialog dlg(parent);
	if (dlg.DoModal() == CMD_OK)
	{
		wcmConfig.editSavePos = dlg.saveFilePosButton.IsSet();
		wcmConfig.editAutoIdent = dlg.autoIdentButton.IsSet();
		wcmConfig.editShl = dlg.shlButton.IsSet();
		
		int tabSize = atoi(unicode_to_utf8(dlg.tabEdit.GetText().ptr()).ptr());
		if (tabSize>0 && tabSize<=64)
			wcmConfig.editTabSize = tabSize;
		return true;
	}
	return false;
}


////////////////////////// StyleOptDialog

class StyleOptDialog: public NCVertDialog {
	void RefreshFontInfo();
	Layout iL;
public:	
	struct Node {
		carray<char> name;
		cfont *oldFont;
		carray<char> *pUri;
		cptr<cfont> newFont;
		bool fixed;
		Node():oldFont(0){}
		Node(const char *n, bool fix,  cfont *old, carray<char> *uri):name(new_char_str(n)), fixed(fix), oldFont(old), pUri(uri){}
	};
	
	ccollect<Node> *pList;

	HKStaticLine styleStatic;

	wal::ccollect< wal::carray<char> > styleList;
	ComboBox styleBox;
	const char *StyleName();

	StaticLine showStatic;
	SButton  showToolbarButton;
	SButton  showButtonbarButton;
	
	HKStaticLine fontsStatic;
	TextList fontList;
	StaticLine fontNameStatic;
	Button changeButton;
	Button changeX11Button;

	StyleOptDialog(NCDialogParent *parent, ccollect<Node> *p);
	virtual bool EventChildKey(Win* child, cevent_key* pEvent);
	virtual bool Command(int id, int subId, Win *win, void *data);
	virtual ~StyleOptDialog();
};

StyleOptDialog::~StyleOptDialog(){}

const char *StyleOptDialog::StyleName()
{
	int n = styleBox.Current();
	return (char *) styleBox.ItemData(n);
}

void StyleOptDialog::RefreshFontInfo()
{
	int count = pList->count();
	int cur = fontList.GetCurrent();
	
	const char *s = "";
	if (count>=0 && cur>=0 && cur<count)
	{
		int n = fontList.GetCurrentInt();
		if (pList->get(n).newFont.ptr())
			s = pList->get(n).newFont->printable_name();
		else
		if (pList->get(n).oldFont)
			s = pList->get(n).oldFont->printable_name();
	}
		
	fontNameStatic.SetText(utf8_to_unicode(s).ptr());
}

#define CMD_CHFONT 1000
#define CMD_CHFONTX11 1001

StyleOptDialog::StyleOptDialog(NCDialogParent *parent, ccollect<Node> *p)
:	NCVertDialog(::createDialogAsChild, 0, parent, utf8_to_unicode( _LT("Style") ).ptr(), bListOkCancel),
	iL(16, 3),
	pList(p),
	styleStatic(0, this, utf8_to_unicode( _LT("&Colors:") ).ptr()),
	styleBox(0, this, 10, 3, ComboBox::READONLY | ComboBox::FRAME3D ),

showStatic(0, this, utf8_to_unicode( _LT("Items:") ).ptr()),
showToolbarButton(0, this, utf8_to_unicode( _LT("Show &toolbar") ).ptr(), 0, wcmConfig.showToolBar),
showButtonbarButton(0, this, utf8_to_unicode( _LT("Show &buttonbar") ).ptr(), 0, wcmConfig.showButtonBar),

	fontsStatic(0, this, utf8_to_unicode( _LT("&Fonts:") ).ptr()),
	fontList(Win::WT_CHILD, WH_TABFOCUS | WH_CLICKFOCUS, 0, this, VListWin::SINGLE_SELECT, VListWin::BORDER_3D, 0),
	fontNameStatic(0, this, utf8_to_unicode("--------------------------------------------------").ptr()),
	changeButton(0, this, utf8_to_unicode( _LT("&Set font...") ).ptr(), CMD_CHFONT),
	changeX11Button(0, this, utf8_to_unicode( _LT("Set &X11 font...") ).ptr(), CMD_CHFONTX11)
{
	styleStatic.SetControlWin(&styleBox);
	iL.AddWin(&styleStatic,	1, 0 ); styleStatic.Enable(); styleStatic.Show(); 

	GetColorStyleList(&styleList);
	for (int i = 0; i < styleList.count(); i++)
		styleBox.Append( _LT(styleList[i].ptr()), styleList[i].ptr() );

	if (currentColorStyleName)
	{
		for (int i = 0; i < styleList.count(); i++)
			if (!strcmp( styleList[i].ptr(), currentColorStyleName) ) 
			{
				styleBox.MoveCurrent(i);
				break;
			}
	}

	iL.AddWin(&styleBox, 1, 1); 
	styleBox.Enable(); 
	styleBox.Show();

	iL.AddWin(&showStatic,	4, 0 ); showStatic.Enable(); showStatic.Show(); 
	iL.AddWin(&showToolbarButton,	5, 1); showToolbarButton.Enable(); showToolbarButton.Show(); 
	iL.AddWin(&showButtonbarButton,	6, 1); showButtonbarButton.Enable(); showButtonbarButton.Show(); 


	iL.LineSet(7,10);
	iL.AddWin(&fontsStatic,	8, 0); fontsStatic.Enable(); fontsStatic.Show(); 
	fontsStatic.SetControlWin(&fontList);

	iL.ColSet(0,10,10,10);
	iL.SetColGrowth(1);
	
	for (int i = 0; i<pList->count(); i++)
		fontList.Append(utf8_to_unicode(pList->get(i).name.ptr()).ptr(), i);
		
	fontList.MoveCurrent(0);
	RefreshFontInfo();
	
	LSize lsize = fontList.GetLSize();
	lsize.y.minimal = lsize.y.ideal = 100;
	lsize.y.maximal = 1000;
	lsize.x.minimal = lsize.x.ideal = 200;
	lsize.x.maximal = 1000;
	fontList.SetLSize(lsize);
	
	iL.AddWin(&fontList, 8, 1); fontList.Enable(); fontList.Show(); 
	
	fontNameStatic.Enable(); fontNameStatic.Show(); 
	
	lsize = fontNameStatic.GetLSize();
	lsize.x.minimal = 400;
	lsize.x.maximal = 1000;
	fontNameStatic.SetLSize(lsize);
	
	iL.AddWin(&fontNameStatic, 9, 1);
#ifdef USEFREETYPE	
	iL.AddWin(&changeButton, 10, 1); changeButton.Enable(); changeButton.Show(); 
#endif	

#ifdef _WIN32
	iL.AddWin(&changeButton, 10, 1); changeButton.Enable(); changeButton.Show(); 
#else
	iL.AddWin(&changeX11Button, 11, 1); changeX11Button.Enable(); changeX11Button.Show(); 
	iL.LineSet(12,10);
#endif

#ifdef USEFREETYPE			
	LSize l1 = changeButton.GetLSize();
	LSize l2 = changeX11Button.GetLSize();
	if (l1.x.minimal < l2.x.minimal) l1.x.minimal = l2.x.minimal;
	l1.x.maximal = l1.x.minimal;
	changeButton.SetLSize(l1);
	changeX11Button.SetLSize(l1);
#endif	

	AddLayout(&iL);
	SetEnterCmd(CMD_OK);
	styleBox.SetFocus();
	order.append(&styleBox);
	order.append(&showToolbarButton);
	order.append(&showButtonbarButton);
	order.append(&fontList);
	order.append(&changeButton);
	order.append(&changeX11Button);
	SetPosition();
}

bool StyleOptDialog::Command(int id, int subId, Win *win, void *data)
{
	
	if (win == &fontList)
	{
		RefreshFontInfo();
		return true;
	}
#ifdef _WIN32
	if (id == CMD_CHFONT)
	{
		int count = pList->count();
		int cur = fontList.GetCurrent();
	
		if (count<=0 || cur<0 || cur>=count) return true;

		LOGFONT lf;
		carray<char> *pUri = pList->get(fontList.GetCurrentInt()).pUri;
		cfont::UriToLogFont(&lf, pUri && pUri->ptr() ?  pUri->ptr() : 0);

		CHOOSEFONT cf;
		memset(&cf,0,sizeof(cf));
		cf.lStructSize = sizeof(cf);
		cf.hwndOwner = GetID();
		cf.lpLogFont = &lf;
		cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT ;

		if (pList->get(fontList.GetCurrentInt()).fixed)
			cf.Flags |= CF_FIXEDPITCHONLY;


		if (ChooseFont(&cf))
		{
			cptr<cfont> p = new cfont(cfont::LogFontToUru(lf).ptr());
			if (p.ptr()) {
				pList->get(fontList.GetCurrentInt()).newFont = p;
				RefreshFontInfo();
			}
		}

		return true;
	}

#else
	if (id == CMD_CHFONT)
	{
		int count = pList->count();
		int cur = fontList.GetCurrent();
	
		if (count<=0 || cur<0 || cur>=count) return true;
		
		carray<char> *pUri = pList->get(fontList.GetCurrentInt()).pUri;
		
		cptr<cfont> p = SelectFTFont((NCDialogParent*)Parent(), pList->get(fontList.GetCurrentInt()).fixed, (pUri && pUri->ptr()) ? pUri->ptr() : 0 );
		if (p.ptr()) {
			pList->get(fontList.GetCurrentInt()).newFont = p;
			RefreshFontInfo();
		}
		
		return true;
	}
	
	if (id == CMD_CHFONTX11)
	{
		int count = pList->count();
		int cur = fontList.GetCurrent();
	
		if (count<=0 || cur<0 || cur>=count) return true;

		cptr<cfont> p = SelectX11Font((NCDialogParent*)Parent(), pList->get(fontList.GetCurrentInt()).fixed);
		if (p.ptr()) {
			pList->get(fontList.GetCurrentInt()).newFont = p;
			RefreshFontInfo();
		}
		
		return true;
	}
#endif
	
	return NCVertDialog::Command(id, subId, win, data);
}

bool StyleOptDialog::EventChildKey(Win* child, cevent_key* pEvent)
{
	if (pEvent->Type() == EV_KEYDOWN) 
	{
		if (
			pEvent->Key() == VK_RETURN && (child == &changeButton || child == &changeX11Button) ||
			(pEvent->Key() == VK_UP || pEvent->Key() == VK_DOWN) && child == &fontList
		) {
			return false;
		}
	}; 

 	return NCVertDialog::EventChildKey(child, pEvent);
}

bool DoStyleConfigDialog(NCDialogParent *parent)
{
	wcmConfig.ImpCurrentFonts();
	ccollect<StyleOptDialog::Node> list;
	list.append(StyleOptDialog::Node(  _LT("Panel") ,	false,	panelFont.ptr(), &wcmConfig.panelFontUri));
	list.append(StyleOptDialog::Node(  _LT("Dialog"),	false,	dialogFont.ptr(), &wcmConfig.dialogFontUri));
	list.append(StyleOptDialog::Node(  _LT("Viewer"),	true, 	viewerFont.ptr(), &wcmConfig.viewerFontUri));
	list.append(StyleOptDialog::Node(  _LT("Editor"),	true, 	editorFont.ptr(), &wcmConfig.editorFontUri));
	list.append(StyleOptDialog::Node(  _LT("Terminal"),	true,	terminalFont.ptr(), &wcmConfig.terminalFontUri));
	list.append(StyleOptDialog::Node(  _LT("Help text"),	false,	helpTextFont.ptr(), &wcmConfig.helpTextFontUri));
	list.append(StyleOptDialog::Node(  _LT("Help bold text"),	false,	helpBoldFont.ptr(), &wcmConfig.helpBoldFontUri));
	list.append(StyleOptDialog::Node(  _LT("Help header text"),	false,	helpHeadFont.ptr(), &wcmConfig.helpHeadFontUri));
	
	carray<char> styleErr; 

	{	//чтоб диалого уже не было если ошибка
		StyleOptDialog dlg(parent, &list);

		if (dlg.DoModal() != CMD_OK) return false;

		styleErr = SetColorStyle(dlg.StyleName());
		if (styleErr.ptr() == 0 && dlg.StyleName())
			wcmConfig.style = new_char_str( dlg.StyleName() );

		wcmConfig.showToolBar = dlg.showToolbarButton.IsSet();
		wcmConfig.showButtonBar = dlg.showButtonbarButton.IsSet();
		
		for (int i = 0; i<list.count(); i++)
			if (list[i].newFont.ptr() && list[i].newFont->uri()[0] && list[i].pUri)
			{
				*(list[i].pUri) = new_char_str(list[i].newFont->uri());
			}
	}

	if (styleErr.ptr())
		NCMessageBox(parent, _LT("Bad style info"), styleErr.ptr(), true);

	return true;
}

struct LangListNode {
	carray<char> id;
	carray<char> name;
	LangListNode(){}
	LangListNode(const char *i, const char *n):id(new_char_str(i)), name(new_char_str(n)){}
};

inline bool IsSpace(char c){ return c>0 && c<=0x20; }

static bool LangListLoad(sys_char_t *fileName, ccollect<LangListNode> &list)
{
	list.clear();
	try {
		BFile f;
		f.Open(fileName);
		char buf[4096];
		
		while (f.GetStr(buf, sizeof(buf)))
		{
			char *s = buf;
			while (IsSpace(*s)) s++;
			if (*s == '#') 	continue;

			if (!*s) continue;
			
			ccollect<char,0x100> id;
			ccollect<char,0x100> name;
			
			while (*s && !IsSpace(*s)) {
				id.append(*s);
				s++;
			}
			
			while (IsSpace(*s)) s++;
			
			int lastNs = -1;
			for (int i = 0; *s; i++, s++) {
				if (*s == '#') break;
				if (!IsSpace(*s)) lastNs = i;
				name.append(*s);
			}
			if (id.count()<=0 || lastNs < 0) continue;
			id.append(0);
			name.append(0);
			name[lastNs + 1] = 0; 
			
			LangListNode(id.ptr(), name.ptr());
			list.append(LangListNode(id.ptr(), name.ptr()) );
		}
	} catch (cexception *ex) {
		ex->destroy();
		return false;
	}
	return true;
}


////////////////////////////////  SysOptDlg

class SysOptDialog: public NCVertDialog {
	Layout iL;
public:	
	carray<char> curLangId;
	ccollect<LangListNode> list;
	void SetCurLang(const char *id);

	SButton  askOpenExecButton;
	SButton  escPanelButton;
	SButton  saveHistoryButton;
	SButton  autoCompleteButton;
	HKStaticLine langStatic;
	ComboBox langBox;

	SysOptDialog(NCDialogParent *parent);
	virtual bool Command(int id, int subId, Win *win, void *data);
	virtual bool EventChildKey(Win* child, cevent_key* pEvent);
	virtual ~SysOptDialog();
};


void SysOptDialog::SetCurLang(const char *id)
{
	curLangId = new_char_str(id ? id : "+");
}


SysOptDialog::~SysOptDialog(){}

SysOptDialog::SysOptDialog(NCDialogParent *parent)
:	NCVertDialog(::createDialogAsChild, 0, parent, utf8_to_unicode( _LT("System settings") ).ptr(), bListOkCancel),
	iL(16, 3)

	,askOpenExecButton(0, this, utf8_to_unicode( _LT("&Ask user if Exec/Open conflict") ).ptr(), 0, wcmConfig.systemAskOpenExec)
	,escPanelButton(0, this, utf8_to_unicode( _LT("Enable &ESC key to show/hide panels") ).ptr(), 0, wcmConfig.systemEscPanel)
	,saveHistoryButton(0, this, utf8_to_unicode( _LT("Save &history to disk") ).ptr(), 0, wcmConfig.systemSaveHistory)
	,autoCompleteButton(0, this, utf8_to_unicode( _LT("Enable &auto complete") ).ptr(), 0, wcmConfig.systemAutoComplete)
	,langStatic(0, this, utf8_to_unicode( _LT("&Language:") ).ptr())
	,langBox(0, this, 10, 5, ComboBox::READONLY)
{

#ifndef _WIN32
	iL.AddWin(&askOpenExecButton,	0, 0, 0, 1); askOpenExecButton.Enable();  askOpenExecButton.Show(); 
#endif
	iL.AddWin(&escPanelButton,	1, 0, 1, 1); escPanelButton.Enable();  escPanelButton.Show(); 
	iL.AddWin(&saveHistoryButton,	2, 0, 2, 1); saveHistoryButton.Enable();  saveHistoryButton.Show(); 
	iL.AddWin(&autoCompleteButton,	3, 0, 3, 1); autoCompleteButton.Enable();  autoCompleteButton.Show(); 
	iL.AddWin(&langStatic, 		4, 0);	langStatic.Enable();	langStatic.Show();
	langStatic.SetControlWin(&langBox);
	iL.AddWin(&langBox,		4, 1);	langBox.Enable();	langBox.Show();
	iL.SetColGrowth(2);
	
	AddLayout(&iL);
	SetEnterCmd(CMD_OK);

#ifndef _WIN32
	askOpenExecButton.SetFocus();
	order.append(&askOpenExecButton);
#endif
	order.append(&escPanelButton);
	order.append(&saveHistoryButton);
	order.append(&autoCompleteButton);
	order.append(&langBox);

	SetPosition();

#ifdef _WIN32
	LangListLoad( carray_cat<sys_char_t>(GetAppPath().ptr(), utf8_to_sys("\\lang\\list").ptr()).ptr(), list);
#else
	if (!LangListLoad(utf8_to_sys("install-files/share/wcm/lang/list").ptr(), list))
		LangListLoad(utf8_to_sys(UNIX_CONFIG_DIR_PATH "/lang/list").ptr(), list);
#endif

	static char str_plus[]="+";
	static char str_minus[]="-";

	langBox.Append( _LT("Autodetect"), &str_plus ); //0
	langBox.Append( _LT("English") , &str_minus); //1	

	int i;
	for (i = 0; i <	 list.count(); i++)
		langBox.Append( _LT(list[i].name.ptr()), list[i].id.ptr() );

	SetCurLang(wcmConfig.systemLang.ptr() ? wcmConfig.systemLang.ptr() : "+");

	for (i = 0; i < langBox.Count(); i++)
	{
		char *s = (char*)langBox.ItemData(i);
		if (s && !strcmp(curLangId.ptr(), s)) 
		{
			langBox.MoveCurrent(i);
			break;
		}
	}
}

bool SysOptDialog::Command(int id, int subId, Win *win, void *data)
{
	if (win == &langBox && id == CMD_ITEM_CHANGED) 
	{
		SetCurLang((const char*)langBox.ItemData(subId));
		return true;
	}

	return NCVertDialog::Command(id, subId, win, data);
}

bool SysOptDialog::EventChildKey(Win* child, cevent_key* pEvent)
{
 	return NCVertDialog::EventChildKey(child, pEvent);
}


bool DoSystemConfigDialog(NCDialogParent *parent)
{
	bool langChanged = false;

	{	//////////
		SysOptDialog dlg(parent);

		if (dlg.DoModal() != CMD_OK) return false;

		wcmConfig.systemAskOpenExec = dlg.askOpenExecButton.IsSet(); 
		wcmConfig.systemEscPanel = dlg.escPanelButton.IsSet();
		wcmConfig.systemSaveHistory = dlg.saveHistoryButton.IsSet();
		wcmConfig.systemAutoComplete = dlg.autoCompleteButton.IsSet();
		const char *s = wcmConfig.systemLang.ptr();

		if (!s) s = "+";

		langChanged = strcmp(dlg.curLangId.ptr(), s) != 0;
		wcmConfig.systemLang = new_char_str(dlg.curLangId.ptr());
	}	/////////
		
	if (langChanged) 
	{
		NCMessageBox(parent, _LT("Note"), 
			_LT("Language changed. \nFor effect you must save config and restart"), false);
	}
		
	return true;
}


////////////////////////// TerminalOptDialog

class TerminalOptDialog: public NCVertDialog {
	Layout iL;
public:	
	StaticLine backspaceKeyStatic;
	SButton  backspaceAsciiButton;
	SButton  backspaceCtrlHButton;
	
	TerminalOptDialog(NCDialogParent *parent);
	//virtual bool EventChildKey(Win* child, cevent_key* pEvent);
	//virtual bool Command(int id, int subId, Win *win, void *data);
	virtual ~TerminalOptDialog();
};


TerminalOptDialog::TerminalOptDialog(NCDialogParent *parent)
:	NCVertDialog(::createDialogAsChild, 0, parent, utf8_to_unicode( _LT("Terminal options") ).ptr(), bListOkCancel),
	iL(16, 3),
	backspaceKeyStatic(0, this, utf8_to_unicode( _LT("Backspace key:") ).ptr()),
	backspaceAsciiButton(0, this, utf8_to_unicode("ASCII DEL").ptr(), 1, wcmConfig.terminalBackspaceKey == 0),
	backspaceCtrlHButton(0, this,  utf8_to_unicode("Ctrl H").ptr(), 1, wcmConfig.terminalBackspaceKey == 1)
{
	iL.AddWin(&backspaceKeyStatic,	0, 0, 0, 1 ); backspaceKeyStatic.Enable(); backspaceKeyStatic.Show(); 
	iL.AddWin(&backspaceAsciiButton,	1, 1); backspaceAsciiButton.Enable(); backspaceAsciiButton.Show(); 
	iL.AddWin(&backspaceCtrlHButton,	2, 1); backspaceCtrlHButton.Enable(); backspaceCtrlHButton.Show(); 

	iL.ColSet(0,10,10,10);
	iL.SetColGrowth(1);
	
	AddLayout(&iL);
	SetEnterCmd(CMD_OK);

	backspaceAsciiButton.SetFocus();
	
	order.append(&backspaceAsciiButton);
	order.append(&backspaceCtrlHButton);
	SetPosition();
}

TerminalOptDialog::~TerminalOptDialog(){}


bool DoTerminalConfigDialog(NCDialogParent *parent)
{
	TerminalOptDialog dlg(parent);

	if (dlg.DoModal() == CMD_OK)
	{
		wcmConfig.terminalBackspaceKey = dlg.backspaceCtrlHButton.IsSet() ? 1 : 0; 
		return true;
	}

	return false;
}


#ifndef _WIN32
int TerminalBackspaceKey()
{
	return wcmConfig.terminalBackspaceKey ? 8 : 127;
}
#endif
