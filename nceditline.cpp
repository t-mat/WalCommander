#include "nceditline.h"
#include "wcm-config.h"
#include "string-util.h"

//typedef ccollect<carray<unicode_t> > AcCollect;

static cstrhash< cptr< HistCollect > > histHash;

inline bool HistoryCanSave(){ return wcmConfig.systemSaveHistory; } 
inline bool AcEnabled(){ return wcmConfig.systemAutoComplete; } 

static carray<char> HistSectionName(const char *acGroup)
{
	return carray_cat<char>("hist-", acGroup);
}

HistCollect* HistGetList(const char *histGroup)
{
	if (!histGroup || !histGroup[0]) return 0;

	if (HistoryCanSave())
	{
		ccollect< carray<char> > list;
		cptr< HistCollect > pUList = new HistCollect;
		LoadStringList(HistSectionName(histGroup).ptr(), list);

		for (int i = 0; i<list.count(); i++)
		{
			if (list[i].ptr() && list[i][0])
				pUList->append( utf8_to_unicode(list[i].ptr()) );
		}

		histHash[histGroup] = pUList;
	}
	
	cptr<HistCollect> * pp = histHash.exist(histGroup);
	if (pp && pp->ptr()) return pp->ptr();

	cptr< HistCollect > pUList = new HistCollect;
	HistCollect *pList = pUList.ptr();
	histHash[histGroup] = pUList;

	return pList;
}

static bool HistStrcmp(const unicode_t *a, const unicode_t *b)
{
	for (; *a && *b && *a == *b; a++, b++) { /**/ };
	
	while (*a && (*a == ' ' || *a == '\t')) a++;
	while (*b && (*b == ' ' || *b == '\t')) b++;
	return !*a && !*b;
}

static void HistCommit(const char *histGroup, const unicode_t *txt)
{
	if (!histGroup || !histGroup[0]) return;

	if (!txt || !txt[0]) return;

	HistCollect *pList = HistGetList(histGroup);
	cptr< HistCollect > pUList = new HistCollect;
	pUList->append( new_unicode_str(txt) );

	int n = pList->count();
	for (int i = 0; i < n; i++)
		if (!HistStrcmp(txt, pList->get(i).ptr()))
			pUList->append(pList->get(i));

	pList = pUList.ptr();
	histHash[histGroup] = pUList;

	if (HistoryCanSave())
	{
		ccollect< carray<char> > list;
		int n = pList->count();

		if (n > 100) n = 100; //чтоб не до бесконечности сохранялось

		for (int i=0; i<n; i++)
			list.append( unicode_to_utf8(pList->get(i).ptr()) );

		SaveStringList(HistSectionName(histGroup).ptr(), list);
	}
}

int uiClassNCEditLine = GetUiID("NCEditLine");

int NCEditLine::UiGetClassId()
{
	return uiClassNCEditLine;
}

NCEditLine::NCEditLine(const char *acGroup, int nId, Win *parent, const unicode_t *txt, int cols, int rows, bool up, bool frame3d, bool nofocusframe, crect *rect)
:	ComboBox(nId, parent, cols, rows, 
			 (up ? ComboBox::MODE_UP: 0) | (frame3d ? ComboBox::FRAME3D : 0) | (nofocusframe ? ComboBox::NOFOCUSFRAME : 0),
			 rect),
	_group(acGroup),
	_autoMode(false)
{
}

void NCEditLine::Clear()
{
	CloseBox();
	static unicode_t s0 = 0;
	SetText(&s0);
}

bool NCEditLine::OnTextChanged()
{
	if (AcEnabled())
	{
		if (IsBoxOpened() && !_autoMode) return false;
		
		if (!_histList.ptr()) 
			LoadHistoryList();

		carray<unicode_t> text = GetText();
		
		SetCBList(text.ptr());

		unicode_t *u = text.ptr();
		while (*u == ' ' || *u == '\t') u++;

		if (ComboBox::Count() > 0 && *u) 
		{
			_autoMode = true;
			OpenBox();
		} else 
			CloseBox();
	}
	return true;
}

bool NCEditLine::Command(int id, int subId, Win *win, void *d)
{
	if (IsEditLine(win))
	{
		return OnTextChanged();
/*		
		if (AcEnabled())
		{
			if (IsBoxOpened() && !_autoMode) return false;

			if (!_histList.ptr()) 
				LoadHistoryList();

			carray<unicode_t> text = GetText();

			SetCBList(text.ptr());

			unicode_t *u = text.ptr();
			while (*u == ' ' || *u == '\t') u++;

			if (ComboBox::Count() > 0 && *u) 
			{
				_autoMode = true;
				OpenBox();
			} else 
				CloseBox();
		}
		return true;
*/		
	};

	return ComboBox::Command(id, subId, win, d);
}


static bool AcEqual(const unicode_t *txt, const unicode_t *element)
{
	if (!txt) return true;
	if (!element) return false;

	for (;*txt && *element && *txt == *element; txt++, element++) { /**/ };
	return *txt == 0;
}

void NCEditLine::SetCBList(const unicode_t *txt)
{
	ComboBox::Clear();
	if (!_histList.ptr()) return;
	int n = _histList->count();

	for (int i = 0; i<n; i++)
	{
		const unicode_t *u = _histList->get(i).ptr();
		if (u && AcEqual(txt, u)) ComboBox::Append(u);
	}

	ComboBox::MoveCurrent(-1);
	ComboBox::RefreshBox();
}

void NCEditLine::LoadHistoryList()
{
	HistCollect *p = HistGetList(_group);
	if (p) {
		_histList = new HistCollect;
		int n = p->count();
		for (int i = 0; i<n; i++)
			_histList->append( new_unicode_str(p->get(i).ptr() ) );
	}
}

bool NCEditLine::OnOpenBox()
{
	if (!_group || !_group[0]) return false;

	if (!_histList.ptr()) {
		LoadHistoryList();
	}

	SetCBList(_autoMode ? GetText().ptr() : 0);

	return true;
}

void NCEditLine::OnCloseBox()
{
	_autoMode = false;
}

void NCEditLine::Commit()
{
	HistCommit(_group, GetText().ptr());
	_histList = 0;
}

void NCEditLine::SetText(const unicode_t *txt, bool mark)
{
	ComboBox::SetText(txt, mark);
	CloseBox();
	//OnTextChanged();
}

void NCEditLine::InsertText(unicode_t t)
{
	ComboBox::InsertText(t);
	OnTextChanged();
}

void NCEditLine::InsertText(const unicode_t *txt)
{
	ComboBox::InsertText(txt);
	OnTextChanged();
}


NCEditLine::~NCEditLine(){}

