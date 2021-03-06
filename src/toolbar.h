/*
 * Part of Wal Commander GitHub Edition
 * https://github.com/corporateshark/WalCommander
 * walcommander@linderdaum.com
 */

#pragma once

#include "swl.h"

using namespace wal;

class ToolBar: public Win
{
	struct Node: public iIntrusiveCounter
	{
		crect rect;
		int cmd;
		clPtr<cicon> icon;
		std::vector<unicode_t> tipText;
		Node(): rect( 0, 0, 0, 0 ), cmd( 0 ) {}
	};
	void RecalcItems();

	enum DrawStates
	{
		DRAW_NORMAL = 0,
		DRAW_SELECTED = 1,
		DRAW_PRESSED = 2
	};

	Node* GetNodeByPos( int x, int y );

	void DrawNode( wal::GC& gc, Node* pNode, int state );
	int _iconSize;
	std::vector< clPtr<Node> > _list; //null node is splitter
	Node* _pressed;

	int _ticks;
	cpoint _tipPoint;
	Node* _curTip, *_nextTip;
public:
	ToolBar( Win* parent, const crect* rect = 0, int iconSize = 16 );
	virtual void Paint( wal::GC& gc, const crect& paintRect );
	virtual void OnChangeStyles();
	virtual bool EventMouse( cevent_mouse* pEvent );
	virtual void EventEnterLeave( cevent* pEvent );
	virtual void EventTimer( int tid );
	virtual int UiGetClassId();
	void Clear();
	void AddCmd( int cmd, const char* tipText );
	void AddSplitter();
	virtual ~ToolBar();
};
