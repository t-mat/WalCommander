#ifndef NCEDITLINE_H
#define NCEDITLINE_H
#include <swl.h>

using namespace wal;

typedef ccollect<carray<unicode_t> > HistCollect;

// editline with history and autocomplete
class NCEditLine: public ComboBox {
	cptr<HistCollect> _histList;
	const char *_group;
	bool _autoMode;
	void SetCBList(const unicode_t *txt);
	void LoadHistoryList();
public:
	NCEditLine(const char *acGroup, int nId, Win *parent, const unicode_t *txt, int cols, int rows, bool up, bool frame3d, bool nofocusframe,  crect *rect = 0);
	virtual bool Command(int id, int subId, Win *win, void *d);
	void Clear();
	bool OnTextChanged();
	void SetText(const unicode_t *txt, bool mark = false);
	
	void InsertText(unicode_t t);
	void InsertText(const unicode_t *txt);

	virtual int UiGetClassId();
	virtual bool OnOpenBox();
	virtual void OnCloseBox();
	void Commit();
	virtual ~NCEditLine();
};

HistCollect* HistGetList(const char *histGroup);

#endif
