/*
	Copyright (c) by Valery Goryachev (Wal)
*/


#include "swl.h"

namespace wal {

#define LEFTSPACE 5
#define RIGHTSPACE 5
#define ICONX_RIGHTSPACE 1

int uiClassButton = GetUiID("Button");

int Button::UiGetClassId()
{
	return uiClassButton;
}

void Button::OnChangeStyles()
{
	GC gc(this);
	gc.Set(GetFont());
	cpoint p = HkStringGetTextExtents(gc, text.ptr());

	if (icon.ptr()) 
	{
		if (p.y < 12)
			p.y = 12;
				
		p.x += ICONX_RIGHTSPACE + icon->Width();
		if (icon->Width() > p.y+4) 
			p.y = icon->Width()-4;
	}

	p.x += LEFTSPACE + RIGHTSPACE;
	p.x += 8;
	p.y += 8;
	SetLSize(LSize(p));
}

static unicode_t spaceUnicodeStr[]={' ', 0};

Button::Button(int nId, Win *parent, const unicode_t *txt, int id, crect *rect, int iconX, int iconY)
:	Win(Win::WT_CHILD,Win::WH_TABFOCUS|WH_CLICKFOCUS,parent, rect, nId),
	pressed(false), 
	icon(new cicon(id, iconX, iconY)),
	commandId(id)
{
	if (!icon->Valid()) 
		icon.clear();
               
	text = new_unicode_str(txt && txt[0] ? txt : spaceUnicodeStr);
	
	key = HkStringKey(text.ptr());

	if (!rect)
	{
		OnChangeStyles();
	}
};

void Button::Set(const unicode_t *txt, int id, int iconX, int iconY)
{
	text = new_unicode_str(txt && txt[0] ? txt : spaceUnicodeStr);
	
	key = HkStringKey(text.ptr());

	commandId = id;
	icon = new cicon(id, iconX, iconY);
	if (!icon->Valid())
		icon.clear();
}

bool Button::EventMouse(cevent_mouse* pEvent)
{
	switch (pEvent->Type())	{
	case EV_MOUSE_MOVE:
		if (IsCaptured()) 
		{
			crect r = ClientRect();
			cpoint p = pEvent->Point();
			if (pressed)
			{
				if (p.x<0 || p.y<0 || p.x>=r.right || p.y>=r.bottom) 
				{
					pressed = false;
					Invalidate();
				}
			} else {
				if (p.x>=0 && p.y>=0 && p.x<r.right && p.y<r.bottom) 
				{
					pressed = true;
					Invalidate();
				}
			}
		}
		break;

	case EV_MOUSE_PRESS:
	case EV_MOUSE_DOUBLE:
		if (pEvent->Button()!=MB_L) break;
		SetCapture();
		pressed = true;
		Invalidate();
		break;

	case EV_MOUSE_RELEASE:
		if (pEvent->Button()!=MB_L) break;
		ReleaseCapture();
		if (pressed) SendCommand();
		pressed = false;
		Invalidate();
		break;
	};
	return true;
}

bool Button::EventFocus(bool recv)
{
	bool ret = Win::EventFocus(recv);
	if (!recv && pressed)
		pressed=false;
	Invalidate();
	return ret;
}

bool Button::EventKey(cevent_key* pEvent)
{
	if (pEvent->Type() == EV_KEYDOWN && pEvent->Key() == VK_RETURN)
	{
		pressed = true;
		Invalidate();
		return true;
	}
	if (pressed && pEvent->Type() == EV_KEYUP && pEvent->Key() == VK_RETURN)
	{
		pressed = false;
		Invalidate();
		SendCommand();
		return true;
	}
	return false;
}

bool Button::EventKeyPost(Win *focusWin, cevent_key* pEvent)
{
	if (!key || pEvent->Type() != EV_KEYDOWN) return false;
	if (UnicodeUC(key) != UnicodeUC(pEvent->Char())) return false;
	SetFocus();
	SendCommand();
	return true;
}


void Button::Paint(GC &gc, const crect &paintRect)
{
	unsigned colorBg = UiGetColor(uiBackground, 0, 0, 0x808080); 
	bool mode3d = UiGetBool(ui3d, 0, 0, true);
	crect cr = this->ClientRect();
	crect rect = cr;
	DrawBorder(gc, rect, ColorTone(colorBg, +20));
	rect.Dec();
	DrawBorder(gc, rect, ColorTone(colorBg, -200));
	rect.Dec();

	if (pressed)
	{
		if (mode3d) {
			Draw3DButtonW2(gc, rect, colorBg, false);
			rect.Dec();
			rect.Dec();
		}
	} else {
		if (mode3d) {
			Draw3DButtonW2(gc, rect, colorBg, true);
			rect.Dec();
		} else {
			DrawBorder(gc, rect, colorBg);
			rect.Dec();
			DrawBorder(gc, rect, colorBg);
		}
		
		if (InFocus()) {
			DrawBorder(gc, rect, UiGetColor(uiFocusFrameColor, 0, 0, 0)); 
		} 
		
		rect.Dec();
	}

	gc.SetFillColor(colorBg);
	gc.FillRect(rect);
//	gc.SetTextColor(UiGetColor(uiColor, 0, 0, 0) );

	gc.Set(GetFont());
	cpoint tsize = HkStringGetTextExtents(gc, text.ptr());

	int l = tsize.x + (icon.ptr() ? icon->Width() + ICONX_RIGHTSPACE : 0);
	int w = rect.Width();
	int x = rect.left + (w > l ? (w - l)/2 : 0) +(pressed?2:0);
	
	if (icon.ptr())
	{
		gc.DrawIcon(x, rect.top + (rect.Height() - icon->Height())/2 + (pressed?2:0), icon.ptr());
		x += icon->Width() + ICONX_RIGHTSPACE;
	} 
	gc.SetClipRgn(&rect);

	int color_text = UiGetColor(uiColor, 0, 0, 0x0);
	int color_hotkey = UiGetColor(uiHotkeyColor, 0, 0, 0xFF);

	//gc.TextOutF(x, rect.top + (rect.Height()-tsize.y)/2+(pressed?2:0),text.ptr());
	HkStringTextOutF(gc, x, rect.top + (rect.Height() - tsize.y)/2 + (pressed?2:0), text.ptr(), color_text, color_hotkey);
}

Button::~Button(){}

}; //namespace wal
