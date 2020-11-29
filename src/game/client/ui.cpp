/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/input.h>
#include "ui.h"

/********************************************************
 UI
*********************************************************/

IGraphics *CUIRect::m_pGraphics = 0;

const vec4 CUI::ms_DefaultTextColor(1.0f, 1.0f, 1.0f, 1.0f);
const vec4 CUI::ms_DefaultTextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
const vec4 CUI::ms_HighlightTextColor(0.0f, 0.0f, 0.0f, 1.0f);
const vec4 CUI::ms_HighlightTextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
const vec4 CUI::ms_TransparentTextColor(1.0f, 1.0f, 1.0f, 0.5f);

CUI::CUI()
{
	m_pHotItem = 0;
	m_pActiveItem = 0;
	m_pLastActiveItem = 0;
	m_pBecommingHotItem = 0;

	m_MouseX = 0;
	m_MouseY = 0;
	m_MouseWorldX = 0;
	m_MouseWorldY = 0;
	m_MouseButtons = 0;
	m_LastMouseButtons = 0;
	m_Enabled = true;

	m_Screen.x = 0;
	m_Screen.y = 0;

	m_NumClips = 0;
}

void CUI::Update(float MouseX, float MouseY, float MouseWorldX, float MouseWorldY)
{
	unsigned MouseButtons = 0;
	if(Enabled())
	{
		if(Input()->KeyIsPressed(KEY_MOUSE_1)) MouseButtons |= 1;
		if(Input()->KeyIsPressed(KEY_MOUSE_2)) MouseButtons |= 2;
		if(Input()->KeyIsPressed(KEY_MOUSE_3)) MouseButtons |= 4;
	}

	m_MouseX = MouseX;
	m_MouseY = MouseY;
	m_MouseWorldX = MouseWorldX;
	m_MouseWorldY = MouseWorldY;
	m_LastMouseButtons = m_MouseButtons;
	m_MouseButtons = MouseButtons;
	m_pHotItem = m_pBecommingHotItem;
	if(m_pActiveItem)
		m_pHotItem = m_pActiveItem;
	m_pBecommingHotItem = 0;
}

bool CUI::KeyPress(int Key) const
{
	return Enabled() && Input()->KeyPress(Key);
}

bool CUI::KeyIsPressed(int Key) const
{
	return Enabled() && Input()->KeyIsPressed(Key);
}

void CUI::ConvertCursorMove(float *pX, float *pY, int CursorType) const
{
	float Factor = 1.0f;
	switch(CursorType)
	{
		case IInput::CURSOR_MOUSE:
			Factor = Config()->m_UiMousesens/100.0f;
			break;
		case IInput::CURSOR_JOYSTICK:
			Factor = Config()->m_UiJoystickSens/100.0f;
			break;
	}
	*pX *= Factor;
	*pY *= Factor;
}

const CUIRect *CUI::Screen()
{
	m_Screen.h = 600;
	m_Screen.w = Graphics()->ScreenAspect()*m_Screen.h;
	return &m_Screen;
}

float CUI::PixelSize()
{
	return Screen()->w/Graphics()->ScreenWidth();
}

void CUI::ClipEnable(const CUIRect *pRect)
{
	if(IsClipped())
	{
		dbg_assert(m_NumClips < MAX_CLIP_NESTING_DEPTH, "max clip nesting depth exceeded");
		const CUIRect *pOldRect = ClipArea();
		CUIRect Intersection;
		Intersection.x = max(pRect->x, pOldRect->x);
		Intersection.y = max(pRect->y, pOldRect->y);
		Intersection.w = min(pRect->x+pRect->w, pOldRect->x+pOldRect->w) - pRect->x;
		Intersection.h = min(pRect->y+pRect->h, pOldRect->y+pOldRect->h) - pRect->y;
		m_aClips[m_NumClips] = Intersection;
	}
	else
	{
		m_aClips[m_NumClips] = *pRect;
	}
	m_NumClips++;
	UpdateClipping();
}

void CUI::ClipDisable()
{
	dbg_assert(m_NumClips > 0, "no clip region");
	m_NumClips--;
	UpdateClipping();
}

const CUIRect *CUI::ClipArea() const
{
	dbg_assert(m_NumClips > 0, "no clip region");
	return &m_aClips[m_NumClips - 1];
}

void CUI::UpdateClipping()
{
	if(IsClipped())
	{
		const CUIRect *pRect = ClipArea();
		const float XScale = Graphics()->ScreenWidth()/Screen()->w;
		const float YScale = Graphics()->ScreenHeight()/Screen()->h;
		Graphics()->ClipEnable((int)(pRect->x*XScale), (int)(pRect->y*YScale), (int)(pRect->w*XScale), (int)(pRect->h*YScale));
	}
	else
	{
		Graphics()->ClipDisable();
	}
}

void CUIRect::HSplitMid(CUIRect *pTop, CUIRect *pBottom, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.h/2;
	const float HalfSpacing = Spacing/2;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut - HalfSpacing;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut + HalfSpacing;
		pBottom->w = r.w;
		pBottom->h = r.h - Cut - HalfSpacing;
	}
}

void CUIRect::HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if (pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut;
	}

	if (pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut;
		pBottom->w = r.w;
		pBottom->h = r.h - Cut;
	}
}

void CUIRect::HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if (pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = r.h - Cut;
	}

	if (pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + r.h - Cut;
		pBottom->w = r.w;
		pBottom->h = Cut;
	}
}


void CUIRect::VSplitMid(CUIRect *pLeft, CUIRect *pRight, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.w/2;
	const float HalfSpacing = Spacing/2;

	if (pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut - HalfSpacing;
		pLeft->h = r.h;
	}

	if (pRight)
	{
		pRight->x = r.x + Cut + HalfSpacing;
		pRight->y = r.y;
		pRight->w = r.w - Cut - HalfSpacing;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if (pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut;
		pLeft->h = r.h;
	}

	if (pRight)
	{
		pRight->x = r.x + Cut;
		pRight->y = r.y;
		pRight->w = r.w - Cut;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if (pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = r.w - Cut;
		pLeft->h = r.h;
	}

	if (pRight)
	{
		pRight->x = r.x + r.w - Cut;
		pRight->y = r.y;
		pRight->w = Cut;
		pRight->h = r.h;
	}
}

void CUIRect::Margin(float Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x + Cut;
	pOtherRect->y = r.y + Cut;
	pOtherRect->w = r.w - 2*Cut;
	pOtherRect->h = r.h - 2*Cut;
}

void CUIRect::VMargin(float Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x + Cut;
	pOtherRect->y = r.y;
	pOtherRect->w = r.w - 2*Cut;
	pOtherRect->h = r.h;
}

void CUIRect::HMargin(float Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x;
	pOtherRect->y = r.y + Cut;
	pOtherRect->w = r.w;
	pOtherRect->h = r.h - 2*Cut;
}

bool CUIRect::Inside(float x, float y) const
{
	return x >= this->x
		&& x < this->x + this->w
		&& y >= this->y
		&& y < this->y + this->h;
}

bool CUI::DoButtonLogic(const void *pID, const CUIRect *pRect, int Button)
{
	// logic
	bool Clicked = false;
	static int s_LastButton = -1;
	const bool Hovered = MouseHovered(pRect);

	if(CheckActiveItem(pID))
	{
		if(s_LastButton == Button && !MouseButton(s_LastButton))
		{
			if(Hovered)
				Clicked = true;
			SetActiveItem(0);
			s_LastButton = -1;
		}
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(Button))
		{
			SetActiveItem(pID);
			s_LastButton = Button;
		}
	}

	if(Hovered && !MouseButton(Button))
		SetHotItem(pID);

	return Clicked;
}

bool CUI::DoPickerLogic(const void *pID, const CUIRect *pRect, float *pX, float *pY)
{
	if(CheckActiveItem(pID))
	{
		if(!MouseButton(0))
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
			SetActiveItem(pID);
	}

	if(MouseHovered(pRect))
		SetHotItem(pID);

	if(!CheckActiveItem(pID))
		return false;

	if(pX)
		*pX = clamp(m_MouseX - pRect->x, 0.0f, pRect->w);
	if(pY)
		*pY = clamp(m_MouseY - pRect->y, 0.0f, pRect->h);

	return true;
}

void CUI::DoLabel(const CUIRect *pRect, const char *pText, float FontSize, EAlignment Align, float LineWidth, bool MultiLine)
{
	// TODO: FIX ME!!!!
	// Graphics()->BlendNormal();

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxLines = MultiLine ? -1 : 1;
	s_Cursor.m_MaxWidth = LineWidth;
	s_Cursor.m_Align = Align;

	switch(Align)
	{
	case CUI::ALIGN_LEFT:
		s_Cursor.m_Align = TEXTALIGN_LEFT;
		s_Cursor.MoveTo(pRect->x, pRect->y);
		break;
	case CUI::ALIGN_CENTER:
		s_Cursor.m_Align = TEXTALIGN_CENTER;
		s_Cursor.MoveTo(pRect->x + pRect->w / 2.0f, pRect->y);
		break;
	case CUI::ALIGN_RIGHT:
		s_Cursor.m_Align = TEXTALIGN_RIGHT;
		s_Cursor.MoveTo(pRect->x + pRect->w, pRect->y);
		break;
	}
	TextRender()->TextOutlined(&s_Cursor, pText, -1);
}

void CUI::DoLabelHighlighted(const CUIRect *pRect, const char *pText, const char *pHighlighted, float FontSize, const vec4 &TextColor, const vec4 &HighlightColor)
{
	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxWidth = pRect->w;
	s_Cursor.MoveTo(pRect->x, pRect->y);

	TextRender()->TextColor(TextColor);
	const char *pMatch = pHighlighted && pHighlighted[0] ? str_find_nocase(pText, pHighlighted) : 0;
	if(pMatch)
	{
		TextRender()->TextDeferred(&s_Cursor, pText, (int)(pMatch - pText));
		TextRender()->TextColor(HighlightColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch, str_length(pHighlighted));
		TextRender()->TextColor(TextColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch + str_length(pHighlighted), -1);
	}
	else
		TextRender()->TextDeferred(&s_Cursor, pText, -1);

	TextRender()->DrawTextOutlined(&s_Cursor);
}

void CUIRect::Draw(const vec4 &Color, float Rounding, int Corners) const
{
	m_pGraphics->TextureClear();

	// TODO: FIX US
	m_pGraphics->QuadsBegin();
	m_pGraphics->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);

	const float r = Rounding;

	IGraphics::CFreeformItem a_FreeformItems[NUM_ROUND_CORNER_SEGMENTS/2*8];
	int NumFreeformItems = 0;
	for(int i = 0; i < NUM_ROUND_CORNER_SEGMENTS; i += 2)
	{
		float a1 = i/(float)NUM_ROUND_CORNER_SEGMENTS * pi/2;
		float a2 = (i+1)/(float)NUM_ROUND_CORNER_SEGMENTS * pi/2;
		float a3 = (i+2)/(float)NUM_ROUND_CORNER_SEGMENTS * pi/2;
		float Ca1 = cosf(a1);
		float Ca2 = cosf(a2);
		float Ca3 = cosf(a3);
		float Sa1 = sinf(a1);
		float Sa2 = sinf(a2);
		float Sa3 = sinf(a3);

		if(Corners&CORNER_TL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+r, y+r,
				x+(1-Ca1)*r, y+(1-Sa1)*r,
				x+(1-Ca3)*r, y+(1-Sa3)*r,
				x+(1-Ca2)*r, y+(1-Sa2)*r);

		if(Corners&CORNER_TR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w-r, y+r,
				x+w-r+Ca1*r, y+(1-Sa1)*r,
				x+w-r+Ca3*r, y+(1-Sa3)*r,
				x+w-r+Ca2*r, y+(1-Sa2)*r);

		if(Corners&CORNER_BL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+r, y+h-r,
				x+(1-Ca1)*r, y+h-r+Sa1*r,
				x+(1-Ca3)*r, y+h-r+Sa3*r,
				x+(1-Ca2)*r, y+h-r+Sa2*r);

		if(Corners&CORNER_BR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w-r, y+h-r,
				x+w-r+Ca1*r, y+h-r+Sa1*r,
				x+w-r+Ca3*r, y+h-r+Sa3*r,
				x+w-r+Ca2*r, y+h-r+Sa2*r);

		if(Corners&CORNER_ITL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x, y,
				x+(1-Ca1)*r, y-r+Sa1*r,
				x+(1-Ca3)*r, y-r+Sa3*r,
				x+(1-Ca2)*r, y-r+Sa2*r);

		if(Corners&CORNER_ITR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w, y,
				x+w-r+Ca1*r, y-r+Sa1*r,
				x+w-r+Ca3*r, y-r+Sa3*r,
				x+w-r+Ca2*r, y-r+Sa2*r);

		if(Corners&CORNER_IBL)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x, y+h,
				x+(1-Ca1)*r, y+h+(1-Sa1)*r,
				x+(1-Ca3)*r, y+h+(1-Sa3)*r,
				x+(1-Ca2)*r, y+h+(1-Sa2)*r);

		if(Corners&CORNER_IBR)
			a_FreeformItems[NumFreeformItems++] = IGraphics::CFreeformItem(
				x+w, y+h,
				x+w-r+Ca1*r, y+h+(1-Sa1)*r,
				x+w-r+Ca3*r, y+h+(1-Sa3)*r,
				x+w-r+Ca2*r, y+h+(1-Sa2)*r);
	}
	m_pGraphics->QuadsDrawFreeform(a_FreeformItems, NumFreeformItems);

	IGraphics::CQuadItem a_QuadItems[9];
	int NumQuadItems = 0;
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+r, y+r, w-r*2, h-r*2); // center
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+r, y, w-r*2, r); // top
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+r, y+h-r, w-r*2, r); // bottom
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x, y+r, r, h-r*2); // left
	a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+w-r, y+r, r, h-r*2); // right

	if(!(Corners&CORNER_TL)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x, y, r, r);
	if(!(Corners&CORNER_TR)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+w, y, -r, r);
	if(!(Corners&CORNER_BL)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x, y+h, r, -r);
	if(!(Corners&CORNER_BR)) a_QuadItems[NumQuadItems++] = IGraphics::CQuadItem(x+w, y+h, -r, -r);

	m_pGraphics->QuadsDrawTL(a_QuadItems, NumQuadItems);
	m_pGraphics->QuadsEnd();
}

void CUIRect::Draw4(const vec4 &ColorTopLeft, const vec4 &ColorTopRight, const vec4 &ColorBottomLeft, const vec4 &ColorBottomRight, float Rounding, int Corners) const
{
	m_pGraphics->TextureClear();
	m_pGraphics->QuadsBegin();

	const float r = Rounding;

	for(int i = 0; i < NUM_ROUND_CORNER_SEGMENTS; i+=2)
	{
		float a1 = i/(float)NUM_ROUND_CORNER_SEGMENTS * pi/2;
		float a2 = (i+1)/(float)NUM_ROUND_CORNER_SEGMENTS * pi/2;
		float a3 = (i+2)/(float)NUM_ROUND_CORNER_SEGMENTS * pi/2;
		float Ca1 = cosf(a1);
		float Ca2 = cosf(a2);
		float Ca3 = cosf(a3);
		float Sa1 = sinf(a1);
		float Sa2 = sinf(a2);
		float Sa3 = sinf(a3);

		if(Corners&CORNER_TL)
		{
			m_pGraphics->SetColor(ColorTopLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+r, y+r,
				x+(1-Ca1)*r, y+(1-Sa1)*r,
				x+(1-Ca3)*r, y+(1-Sa3)*r,
				x+(1-Ca2)*r, y+(1-Sa2)*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_TR)
		{
			m_pGraphics->SetColor(ColorTopRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w-r, y+r,
				x+w-r+Ca1*r, y+(1-Sa1)*r,
				x+w-r+Ca3*r, y+(1-Sa3)*r,
				x+w-r+Ca2*r, y+(1-Sa2)*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_BL)
		{
			m_pGraphics->SetColor(ColorBottomLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+r, y+h-r,
				x+(1-Ca1)*r, y+h-r+Sa1*r,
				x+(1-Ca3)*r, y+h-r+Sa3*r,
				x+(1-Ca2)*r, y+h-r+Sa2*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_BR)
		{
			m_pGraphics->SetColor(ColorBottomRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w-r, y+h-r,
				x+w-r+Ca1*r, y+h-r+Sa1*r,
				x+w-r+Ca3*r, y+h-r+Sa3*r,
				x+w-r+Ca2*r, y+h-r+Sa2*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_ITL)
		{
			m_pGraphics->SetColor(ColorTopLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x, y,
				x+(1-Ca1)*r, y-r+Sa1*r,
				x+(1-Ca3)*r, y-r+Sa3*r,
				x+(1-Ca2)*r, y-r+Sa2*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_ITR)
		{
			m_pGraphics->SetColor(ColorTopRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w, y,
				x+w-r+Ca1*r, y-r+Sa1*r,
				x+w-r+Ca3*r, y-r+Sa3*r,
				x+w-r+Ca2*r, y-r+Sa2*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_IBL)
		{
			m_pGraphics->SetColor(ColorBottomLeft);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x, y+h,
				x+(1-Ca1)*r, y+h+(1-Sa1)*r,
				x+(1-Ca3)*r, y+h+(1-Sa3)*r,
				x+(1-Ca2)*r, y+h+(1-Sa2)*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}

		if(Corners&CORNER_IBR)
		{
			m_pGraphics->SetColor(ColorBottomRight);
			IGraphics::CFreeformItem ItemF = IGraphics::CFreeformItem(
				x+w, y+h,
				x+w-r+Ca1*r, y+h+(1-Sa1)*r,
				x+w-r+Ca3*r, y+h+(1-Sa3)*r,
				x+w-r+Ca2*r, y+h+(1-Sa2)*r);
			m_pGraphics->QuadsDrawFreeform(&ItemF, 1);
		}
	}

	// center
	{
		m_pGraphics->SetColor4(ColorTopLeft, ColorTopRight, ColorBottomLeft, ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y+r, w-r*2, h-r*2);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// top
	{
		m_pGraphics->SetColor4(ColorTopLeft, ColorTopRight, ColorTopLeft, ColorTopRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y, w-r*2, r);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// bottom
	{
		m_pGraphics->SetColor4(ColorBottomLeft, ColorBottomRight, ColorBottomLeft, ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+r, y+h-r, w-r*2, r);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// left
	{
		m_pGraphics->SetColor4(ColorTopLeft, ColorTopLeft, ColorBottomLeft, ColorBottomLeft);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y+r, r, h-r*2);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	// right
	{
		m_pGraphics->SetColor4(ColorTopRight, ColorTopRight, ColorBottomRight, ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w-r, y+r, r, h-r*2);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_TL))
	{
		m_pGraphics->SetColor(ColorTopLeft);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y, r, r);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_TR))
	{
		m_pGraphics->SetColor(ColorTopRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w, y, -r, r);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_BL))
	{
		m_pGraphics->SetColor(ColorBottomLeft);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x, y+h, r, -r);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	if(!(Corners&CORNER_BR))
	{
		m_pGraphics->SetColor(ColorBottomRight);
		IGraphics::CQuadItem ItemQ = IGraphics::CQuadItem(x+w, y+h, -r, -r);
		m_pGraphics->QuadsDrawTL(&ItemQ, 1);
	}

	m_pGraphics->QuadsEnd();
}

float CUI::DrawClientID(float FontSize, vec2 CursorPosition, int ID,
							const vec4& BgColor, const vec4& TextColor)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;

	char aBuf[4];
	str_format(aBuf, sizeof(aBuf), "%d", ID);

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_Align = TEXTALIGN_CENTER;

	vec4 OldColor = TextRender()->GetColor();
	TextRender()->TextColor(TextColor);
	TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
	TextRender()->TextColor(OldColor);

	const float LinebaseY = CursorPosition.y + s_Cursor.BaseLineY();
	const float Width = 1.4f * FontSize;

	CUIRect Rect;
	Rect.x = CursorPosition.x;
	Rect.y = LinebaseY - FontSize + 0.15f * FontSize;
	Rect.w = Width;
	Rect.h = FontSize;
	Rect.Draw(BgColor, 0.25f * FontSize);

	s_Cursor.MoveTo(Rect.x + Rect.w / 2.0f, CursorPosition.y);
	TextRender()->DrawTextPlain(&s_Cursor);

	return Width + 0.2f * FontSize;
}

float CUI::GetClientIDRectWidth(float FontSize)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;
	return 1.4f * FontSize + 0.2f * FontSize;
}
