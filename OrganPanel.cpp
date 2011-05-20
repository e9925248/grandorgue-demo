/*
 * GrandOrgue - free pipe organ simulator based on MyOrgan
 *
 * MyOrgan 1.0.6 Codebase - Copyright 2006 Milan Digital Audio LLC
 * MyOrgan is a Trademark of Milan Digital Audio LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "OrganPanel.h"

#include <algorithm>
#include <wx/event.h>
#include <wx/mstream.h>
#include "GOrgueCoupler.h"
#include "GOrgueDrawStop.h"
#include "GOrgueDisplayMetrics.h"
#include "GOrgueDivisional.h"
#include "GOrgueDivisionalCoupler.h"
#include "GOrgueDrawStop.h"
#include "GOrgueEnclosure.h"
#include "GOrgueGeneral.h"
#include "GOrgueLabel.h"
#include "GOrgueMeter.h"
#include "GOrguePiston.h"
#include "GOrguePushbutton.h"
#include "GOrgueSound.h"
#include "GOrgueStop.h"
#include "GOrgueTremulant.h"
#include "GrandOrgue.h"
#include "GrandOrgueFile.h"
#include "GrandOrgueFrame.h"
#include "GrandOrgueID.h"
#include "KeyConvert.h"
#include "OrganDocument.h"

BEGIN_EVENT_TABLE(OrganPanel, wxPanel)
EVT_ERASE_BACKGROUND(OrganPanel::OnErase)
EVT_LEFT_DOWN(OrganPanel::OnMouseLeftDown)
EVT_LEFT_DCLICK(OrganPanel::OnMouseLeftDown)
EVT_RIGHT_DOWN(OrganPanel::OnMouseRightDown)
EVT_RIGHT_DCLICK(OrganPanel::OnMouseRightDown)
EVT_MOUSEWHEEL(OrganPanel::OnMouseScroll)
EVT_KEY_DOWN(OrganPanel::OnKeyCommand)
EVT_PAINT(OrganPanel::OnPaint)

END_EVENT_TABLE()

extern GrandOrgueFile* organfile;
extern GOrgueSound* g_sound;
extern const unsigned char* ImageLoader_Wood[];
extern int c_ImageLoader_Wood[];

OrganPanel::OrganPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY,wxDefaultPosition, wxDefaultSize,  wxTAB_TRAVERSAL, "panel")
{
}

wxPoint g_points[4][17] = {
  { wxPoint( 0, 0), wxPoint(13, 0), wxPoint(13,31), wxPoint( 0,31), wxPoint( 0, 1), wxPoint(12, 1), wxPoint(12,30), wxPoint( 1,30), wxPoint( 1, 1), },
  { wxPoint( 0, 0), wxPoint(10, 0), wxPoint(10,18), wxPoint(13,18), wxPoint(13,31), wxPoint( 0,31), wxPoint( 0, 1), wxPoint( 9, 1), wxPoint( 9,19), wxPoint(12,19), wxPoint(12,30), wxPoint( 1,30), wxPoint( 1, 1), },
  { wxPoint( 3, 0), wxPoint(13, 0), wxPoint(13,31), wxPoint( 0,31), wxPoint( 0,18), wxPoint( 3,18), wxPoint( 3, 1), wxPoint(12, 1), wxPoint(12,30), wxPoint( 1,30), wxPoint( 1,19), wxPoint( 4,19), wxPoint( 4, 1), },
  { wxPoint( 3, 0), wxPoint(10, 0), wxPoint(10,18), wxPoint(13,18), wxPoint(13,31), wxPoint( 0,31), wxPoint( 0,18), wxPoint( 3,18), wxPoint( 3, 1), wxPoint( 9, 1), wxPoint( 9,19), wxPoint(12,19), wxPoint(12,30), wxPoint( 1,30), wxPoint( 1,19), wxPoint( 4,19), wxPoint( 4, 1), },
};

void TileWood(wxDC& dc, int which, int sx, int sy, int cx, int cy)
{
	int x, y;
	wxMemoryInputStream mem((const char*)ImageLoader_Wood[(which - 1) >> 1], c_ImageLoader_Wood[(which - 1) >> 1]);
	wxImage img(mem, wxBITMAP_TYPE_JPEG);
	if ((which - 1) & 1)
		img = img.Rotate90();
	wxBitmap bmp(img);
	dc.SetClippingRegion(sx, sy, cx, cy);
	for (y = sy & 0xFFFFFF00; y < sy + cy; y += 256)
		for (x = sx & 0xFFFFFF00; x < sx + cx; x += 256)
			dc.DrawBitmap(bmp, x, y, false);
	dc.DestroyClippingRegion();
}

void OrganPanel::OnUpdate(wxView *WXUNUSED(sender), wxObject *hint)
{
	int i, j, k;
	wxFont font = *wxNORMAL_FONT;

	// if (m_clientBitmap.Ok() && !hint)
	// 	return;

	m_clientOrigin = GetClientAreaOrigin();

	if (organfile)
	{
		displayMetrics = organfile->GetDisplayMetrics();
		displayMetrics->Update();
	}

	m_clientBitmap = wxBitmap(displayMetrics->GetScreenWidth(), displayMetrics->GetScreenHeight());
	wxMemoryDC dc;
	dc.SelectObject(m_clientBitmap);

	TileWood(dc, displayMetrics->GetDrawstopBackgroundImageNum(), 0, 0, displayMetrics->GetCenterX(), displayMetrics->GetScreenHeight());
	TileWood(dc, displayMetrics->GetDrawstopBackgroundImageNum(), displayMetrics->GetCenterX() + displayMetrics->GetCenterWidth(), 0, displayMetrics->GetScreenWidth() - (displayMetrics->GetCenterX() + displayMetrics->GetCenterWidth()), displayMetrics->GetScreenHeight());
	TileWood(dc, displayMetrics->GetConsoleBackgroundImageNum(), displayMetrics->GetCenterX(), 0, displayMetrics->GetCenterWidth(), displayMetrics->GetScreenHeight());

	if (displayMetrics->HasPairDrawstopCols())
	{
		for (i = 0; i < (displayMetrics->NumberOfDrawstopColsToDisplay() >> 2); i++)
		{
			TileWood(dc,
				displayMetrics->GetDrawstopInsetBackgroundImageNum(),
				i * 174 + displayMetrics->GetJambLeftX() - 5,
				displayMetrics->GetJambLeftRightY(),
				166,
				displayMetrics->GetJambLeftRightHeight());
			TileWood(dc,
				displayMetrics->GetDrawstopInsetBackgroundImageNum(),
				i * 174 + displayMetrics->GetJambRightX() - 5,
				displayMetrics->GetJambLeftRightY(),
				166,
				displayMetrics->GetJambLeftRightHeight());
		}
	}

	if (displayMetrics->HasTrimAboveExtraRows())
		TileWood(dc,
			displayMetrics->GetKeyVertBackgroundImageNum(),
			displayMetrics->GetCenterX(),
			displayMetrics->GetCenterY(),
			displayMetrics->GetCenterWidth(),
			8);

	if (displayMetrics->GetJambTopHeight() + displayMetrics->GetPistonTopHeight())
		TileWood(dc,
			displayMetrics->GetKeyHorizBackgroundImageNum(),
			displayMetrics->GetCenterX(),
			displayMetrics->GetJambTopY(),
			displayMetrics->GetCenterWidth(),
			displayMetrics->GetJambTopHeight() + displayMetrics->GetPistonTopHeight());

	for (i = 0; i <= organfile->GetManualAndPedalCount(); i++)
	{

		TileWood(dc,
			displayMetrics->GetKeyVertBackgroundImageNum(),
			displayMetrics->GetCenterX(),
			organfile->GetManual(i)->m_Y,
			displayMetrics->GetCenterWidth(),
			organfile->GetManual(i)->m_Height);

		TileWood(dc,
			displayMetrics->GetKeyHorizBackgroundImageNum(),
			displayMetrics->GetCenterX(),
			organfile->GetManual(i)->m_PistonY,
			displayMetrics->GetCenterWidth(),
			(!i && displayMetrics->HasExtraPedalButtonRow()) ? 80 : 40);

		if (i < organfile->GetFirstManualIndex())
			continue;

		font = displayMetrics->GetControlLabelFont();
		for (j = 0; j < organfile->GetManual(i)->GetStopCount(); j++)
		{
			if (organfile->GetManual(i)->GetStop(j)->Displayed)
			{
				font.SetPointSize(organfile->GetManual(i)->GetStop(j)->DispLabelFontSize);
				dc.SetFont(font);
				WrapText(dc, organfile->GetManual(i)->GetStop(j)->Name, 51);
			}
		}

		for (j = 0; j < organfile->GetManual(i)->GetCouplerCount(); j++)
		{
			if (organfile->GetManual(i)->GetCoupler(j)->Displayed)
			{
				font.SetPointSize(organfile->GetManual(i)->GetCoupler(j)->DispLabelFontSize);
				dc.SetFont(font);
				WrapText(dc, organfile->GetManual(i)->GetCoupler(j)->Name, 51);
			}
		}

		for (j = 0; j < organfile->GetManual(i)->GetDivisionalCount(); j++)
		{
			if (organfile->GetManual(i)->GetDivisional(j)->Displayed)
			{
				font.SetPointSize(organfile->GetManual(i)->GetDivisional(j)->DispLabelFontSize);
				dc.SetFont(font);
				WrapText(dc, organfile->GetManual(i)->GetDivisional(j)->Name, 28);
			}
		}

		wxRegion region;
		for (j = 0; j < organfile->GetManual(i)->GetNumberOfAccessibleKeys(); j++)
		{
			k = organfile->GetManual(i)->GetFirstAccessibleKeyMIDINoteNumber() + j;
			if ( (((k % 12) < 5 && !(k & 1)) || ((k % 12) >= 5 && (k & 1))))
				DrawKey(dc, i, j, false, &region);
		}

		j = 31 + (organfile->GetManual(i)->DispKeyColourInverted << 1);
		if (j == 31 && (organfile->GetManual(i)->DispKeyColourWooden || !i))
			j = 35;

		if (!region.IsEmpty())
		{
			dc.SetClippingRegion(region);
			TileWood(dc,
				j,
				displayMetrics->GetCenterX(),
				organfile->GetManual(i)->m_KeysY,
				displayMetrics->GetCenterWidth(),
				organfile->GetManual(i)->m_Height);
		}
		region.Clear();

		for (j = 0; j < organfile->GetManual(i)->GetNumberOfAccessibleKeys(); j++)
		{
			k = organfile->GetManual(i)->GetFirstAccessibleKeyMIDINoteNumber() + j;
			if (!(((k % 12) < 5 && !(k & 1)) || ((k % 12) >= 5 && (k & 1))))
				DrawKey(dc, i, j, false, &region);
		}

		j = 33 - (organfile->GetManual(i)->DispKeyColourInverted << 1);
		if (j == 31 && (organfile->GetManual(i)->DispKeyColourWooden || !i))
			j = (displayMetrics->GetKeyVertBackgroundImageNum() % 10) == 1 && !i ? 13 : 35;

		if (!region.IsEmpty())
		{
			dc.SetClippingRegion(region);
			TileWood(dc,
				j,
				displayMetrics->GetCenterX(),
				organfile->GetManual(i)->m_KeysY,
				displayMetrics->GetCenterWidth(),
				organfile->GetManual(i)->m_Height);
		}

		for (k = 0; k < organfile->GetManual(i)->GetNumberOfAccessibleKeys(); k++)
			DrawKey(dc, i, k);

	}

	for (j = 0; j < organfile->GetDivisionalCouplerCount(); j++)
	{
		if (organfile->GetDivisionalCoupler(j)->Displayed)
		{
			font.SetPointSize(organfile->GetDivisionalCoupler(j)->DispLabelFontSize);
			dc.SetFont(font);
			WrapText(dc, organfile->GetDivisionalCoupler(j)->Name, 51);
		}
	}

	for (j = 0; j < organfile->GetTremulantCount(); j++)
	{
		if (organfile->GetTremulant(j)->Displayed)
		{
			font.SetPointSize(organfile->GetTremulant(j)->DispLabelFontSize);
			dc.SetFont(font);
			WrapText(dc, organfile->GetTremulant(j)->Name, 51);
		}
	}

	for (j = 0; j < organfile->GetGeneralCount(); j++)
	{
		if (organfile->GetGeneral(j)->Displayed)
		{
			font.SetPointSize(organfile->GetGeneral(j)->DispLabelFontSize);
			dc.SetFont(font);
			WrapText(dc, organfile->GetGeneral(j)->Name, 28);
		}
	}

	for (j = 0; j < organfile->GetNumberOfReversiblePistons(); j++)
	{
		if (organfile->GetPiston(j)->Displayed)
		{
			font.SetPointSize(organfile->GetPiston(j)->DispLabelFontSize);
			dc.SetFont(font);
			WrapText(dc, organfile->GetPiston(j)->Name, 28);
		}
	}

	j = (displayMetrics->GetScreenWidth() - displayMetrics->GetEnclosureWidth() + 6) >> 1;
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(*wxBLACK_BRUSH);
	for (unsigned l = 0; l < organfile->GetEnclosureCount(); l++)
		organfile->GetEnclosure(l)->DrawLabel(dc);

	DrawClickables(&dc);

	for (j = 0; j < organfile->GetLabelCount(); j++)
		organfile->GetLabel(j)->Draw(dc);

	dc.SelectObject(wxNullBitmap);

	GetParent()->SetClientSize(
		displayMetrics->GetScreenWidth(),
		displayMetrics->GetScreenHeight());

	SetSize(
		displayMetrics->GetScreenWidth(),
		displayMetrics->GetScreenHeight());

	GetParent()->Center(wxBOTH);
	GetParent()->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	GetParent()->Refresh();

}

void OrganPanel::DrawKey(wxDC& dc, int man, int k, bool usepen, wxRegion* region)
{

	GOrgueManual* manual = organfile->GetManual(man);

	if (!manual->Displayed)
		return;

	int x, cx, cy, j, z;
	static int addends[12] = {0, 9, 12, 21, 24, 36, 45, 48, 57, 60, 69, 72};

	if (k < 0 || k > manual->GetNumberOfAccessibleKeys())
		return;

	k += manual->GetFirstAccessibleKeyMIDINoteNumber();
	z  = (((k % 12) < 5 && !(k & 1)) || ((k % 12) >= 5 && (k & 1))) ? 0 : 1;
	cx = 7;
	cy = 20;
	if (man)
	{
		x  = manual->m_X + (k / 12) * 84;
		x += addends[k % 12];
		j  = manual->GetFirstAccessibleKeyMIDINoteNumber();
		x -= (j / 12) * 84;
		x -= addends[j % 12];
		if (!z)
		{
			cx = 13;
			cy = 32;
		}
	}
	else
	{
		cx = 8;
		x  = manual->m_X + (k / 12) * 98;
		x += (k % 12) * 7;
		if ((k % 12) >= 5)
			x += 7;
		j  = manual->GetFirstAccessibleKeyMIDINoteNumber();
		x -= (j / 12) * 98;
		x -= (j % 12) * 7;
		if ((j % 12) >= 5)
			x -= 7;
		if (!z)
			cy = 40;
	}

	wxRegion reg;
	if (!man || z)
	{
		reg.Union(x, manual->m_KeysY, cx + 1, cy);
		z = -4;
	}
	else
	{
		reg.Union(x + 3, manual->m_KeysY, 8, cy);
		reg.Union(x, manual->m_KeysY + 18, 14, 14);
		z = 0;
		j = k % 12;
		if (k > manual->GetFirstAccessibleKeyMIDINoteNumber() && j && j != 5)
			z |= 2;
		if (k < manual->GetFirstAccessibleKeyMIDINoteNumber() + manual->GetNumberOfAccessibleKeys() - 1 && j != 4 && j != 11)
			z |= 1;

		if (!(z & 2))
			reg.Union(x, manual->m_KeysY, 3, 18);
		if (!(z & 1))
			reg.Union(x + 11, manual->m_KeysY, 3, 18);
	}

	if (region)
		region->Union(reg);
	if (usepen)
	{

		const wxPen* pen = manual->IsKeyDown(k) ? wxRED_PEN : wxGREY_PEN;
		dc.SetPen(*pen);
		wxRegion exclude;

		if ((k - manual->GetFirstAccessibleKeyMIDINoteNumber()) > 0 && manual->IsKeyDown(k - 1)) {
			k -= manual->GetFirstAccessibleKeyMIDINoteNumber();
			DrawKey(dc, man, k - 1, 0, &exclude);
			k += manual->GetFirstAccessibleKeyMIDINoteNumber();
		}
		if ((z & 2) && (k - manual->GetFirstAccessibleKeyMIDINoteNumber()) > 1 && manual->IsKeyDown(k - 2)) {
			k -= manual->GetFirstAccessibleKeyMIDINoteNumber();
			DrawKey(dc, man, k - 2, 0, &exclude);
			k += manual->GetFirstAccessibleKeyMIDINoteNumber();
		}
		if ((k - manual->GetFirstAccessibleKeyMIDINoteNumber()) < manual->GetNumberOfAccessibleKeys() - 1 && manual->IsKeyDown(k + 1)) {
			k -= manual->GetFirstAccessibleKeyMIDINoteNumber();
			DrawKey(dc, man, k + 1, 0, &exclude);
			k += manual->GetFirstAccessibleKeyMIDINoteNumber();
		}
		if ((z & 1) && (k - manual->GetFirstAccessibleKeyMIDINoteNumber()) < manual->GetNumberOfAccessibleKeys() - 2 && manual->IsKeyDown(k + 2)) {
			k -= manual->GetFirstAccessibleKeyMIDINoteNumber();
			DrawKey(dc, man, k + 2, 0, &exclude);
			k += manual->GetFirstAccessibleKeyMIDINoteNumber();
		}

		if (!exclude.IsEmpty())
		{
			reg.Subtract(exclude);
			reg.Offset(dc.LogicalToDeviceX(0), dc.LogicalToDeviceY(0));
			dc.SetClippingRegion(reg);
		}
		if (z < 0)
		{
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(x, manual->m_KeysY, cx + 1, cy);
			dc.DrawRectangle(x + 1, manual->m_KeysY + 1, cx - 1, cy - 2);
		}
		else
		{
			dc.DrawPolygon(9 + (((z + 1) >> 1) << 2), g_points[z], x, manual->m_KeysY);
		}
		if (!exclude.IsEmpty())
			dc.DestroyClippingRegion();
	}
}

void OrganPanel::OnErase(wxEraseEvent& event)
{
	if (!m_clientBitmap.Ok() || !organfile || !displayMetrics->GetJambLeftRightWidth())
	{
		event.Skip();
		return;
	}
	wxDC* dc = event.GetDC();
	OnDraw(dc);
}
void OrganPanel::OnPaint(wxPaintEvent& event)
{
	if (!m_clientBitmap.Ok() || !organfile || !displayMetrics->GetJambLeftRightWidth())
	{
		event.Skip();
		return;
	}
	wxPaintDC dc(this);
	OnDraw((wxDC*)&dc);
}

void OrganPanel::OnDrawstop(wxCommandEvent& event)
{
	if (!m_clientBitmap.Ok() || !organfile || !displayMetrics->GetJambLeftRightWidth())
		return;

	wxMemoryDC mdc;
	mdc.SelectObject(m_clientBitmap);
	wxClientDC dc(this);
	dc.SetDeviceOrigin(m_clientOrigin.x, m_clientOrigin.y);

	static_cast<GOrgueDrawable*>(event.GetClientData())->Draw(0, 0, &mdc, &dc);
}

void OrganPanel::OnNoteOnOff(wxCommandEvent& event)
{
	if (!m_clientBitmap.Ok() || !organfile || !displayMetrics->GetJambLeftRightWidth())
		return;

	wxMemoryDC mdc;
	mdc.SelectObject(m_clientBitmap);
	wxClientDC dc(this);
	dc.SetDeviceOrigin(m_clientOrigin.x, m_clientOrigin.y);
	DrawKey(mdc, event.GetInt(), event.GetExtraLong(), true);
	DrawKey( dc, event.GetInt(), event.GetExtraLong(), true);
}

void OrganPanel::OnMouseLeftDown(wxMouseEvent& event)
{
	wxClientDC dc(this);
	DrawClickables(NULL, event.GetX(), event.GetY());
	event.Skip();
}

void OrganPanel::OnMouseRightDown(wxMouseEvent& event)
{
	wxClientDC dc(this);
	DrawClickables(NULL, event.GetX(), event.GetY(), true);
	event.Skip();
}

void OrganPanel::OnMouseScroll(wxMouseEvent& event)
{
	wxClientDC dc(this);
	DrawClickables(
		NULL,
		event.GetX() + m_clientOrigin.x,
		event.GetY() + m_clientOrigin.y,
		false,
		event.GetWheelRotation());
	event.Skip();
}


void OrganPanel::HelpDrawStop(GOrgueDrawstop* stop, wxDC* dc, int xx, int yy, bool right)
{
	if (stop->Draw(xx, yy, dc))
	{
		if (right)
			stop->MIDI();
		else
		{
			stop->Push();
			wxMemoryDC mdc;
			mdc.SelectObject(m_clientBitmap);
			wxClientDC dc(this);
			dc.SetDeviceOrigin(m_clientOrigin.x, m_clientOrigin.y);

			stop->Draw(0, 0, &mdc, &dc);
		}
    }
}

void OrganPanel::HelpDrawButton(GOrguePushbutton* button, wxDC* dc, int xx, int yy, bool right)
{
	if (button->Draw(xx, yy, dc))
	{
		if (right)
			button->MIDI();
		else
		{
			button->Push();
			wxMemoryDC mdc;
			mdc.SelectObject(m_clientBitmap);
			wxClientDC dc(this);
			dc.SetDeviceOrigin(m_clientOrigin.x, m_clientOrigin.y);

			button->Draw(0, 0, &mdc, &dc);
		}
	}
}

void OrganPanel::DrawClickables(wxDC* dc, int xx, int yy, bool right, int scroll)
{
	int i, j;
	if (!m_clientBitmap.Ok())
		return;

	if (!scroll)
	{
		for (i = organfile->GetFirstManualIndex(); i <= organfile->GetManualAndPedalCount(); i++)
		{
			for (j = 0; j < organfile->GetManual(i)->GetStopCount(); j++)
				HelpDrawStop(organfile->GetManual(i)->GetStop(j), dc, xx, yy, right);
			for (j = 0; j < organfile->GetManual(i)->GetCouplerCount(); j++)
				HelpDrawStop(organfile->GetManual(i)->GetCoupler(j), dc, xx, yy, right);
			for (j = 0; j < organfile->GetManual(i)->GetDivisionalCount(); j++)
				HelpDrawButton(organfile->GetManual(i)->GetDivisional(j), dc, xx, yy, right);
			if (dc || !organfile->GetManual(i)->Displayed)
				continue;

			wxRect rect(
				organfile->GetManual(i)->m_X,
				organfile->GetManual(i)->m_Y,
				organfile->GetManual(i)->m_Width,
				organfile->GetManual(i)->m_Height);

			if (rect.Contains(xx, yy))
			{
				if (right)
					organfile->GetManual(i)->MIDI();
			}

		}

		for (j = 0; j < organfile->GetTremulantCount(); j++)
			HelpDrawStop(organfile->GetTremulant(j), dc, xx, yy, right);
		for (j = 0; j < organfile->GetDivisionalCouplerCount(); j++)
			HelpDrawStop(organfile->GetDivisionalCoupler(j), dc, xx, yy, right);
		for (j = 0; j < organfile->GetGeneralCount(); j++)
			HelpDrawButton(organfile->GetGeneral(j), dc, xx, yy, right);
		for (j = 0; j < organfile->GetNumberOfReversiblePistons(); j++)
			HelpDrawButton(organfile->GetPiston(j), dc, xx, yy, right);
	}

	for (unsigned l = 0; l < organfile->GetEnclosureCount(); l++)
	{
		if (organfile->GetEnclosure(l)->Draw(xx, yy, dc))
		{
			if (right)
				organfile->GetEnclosure(l)->MIDI();
			else if (scroll)
				organfile->GetEnclosure(l)->Scroll(scroll > 0);
		}
	}
}

void OrganPanel::WrapText(wxDC& dc, wxString& string, int width)
{
	wxString str = string;

	char *ptr = (char*)str.c_str();
	char *p = ptr, *lastspace = 0;
	wxCoord cx, cy;

	while (p)
	{
		p = strchr(p, ' ');
		if (p)
		{
			*p = 0;
			dc.GetTextExtent(ptr, &cx, &cy);
			*p = ' ';
		}
		else
			dc.GetTextExtent(ptr, &cx, &cy);
		if (cx > width)
		{
			if (lastspace)
			{
				*lastspace = '\n';
				if (p)
					ptr = p = lastspace + 1;
			}
			else
			{
				if (p)
					*p++ = '\n';
				ptr = p;
			}
			lastspace = 0;
		}
		else if (p)
			lastspace = p++;
	}

	lastspace = 0;
	p = ptr = (char*)str.c_str();
	while (*p)
	{
		if (*p == ' ')
		{
			if (!lastspace)
				lastspace = ptr;
		}
		else
		{
			if (*p == '\n' && lastspace)
				ptr = lastspace;
			lastspace = 0;
		}
		*ptr++ = *p++;
	}
	*ptr = 0;

	string = str.c_str();
}


void OrganPanel::OnKeyCommand(wxKeyEvent& event)
{
	if (g_sound && g_sound->b_memset ^ event.ShiftDown())
	{
		::wxGetApp().frame->ProcessCommand(ID_AUDIO_MEMSET);
		UpdateWindowUI();
	}

	int k = event.GetKeyCode();
	if ( !event.AltDown())
	{

		GOrgueMeter* meter = ::wxGetApp().frame->m_meters[2];
		OrganDocument* doc = (OrganDocument*)::wxGetApp().m_docManager->GetCurrentDocument();
		switch(k)
		{
			case WXK_ESCAPE:
			{
				::wxGetApp().frame->ProcessCommand(ID_AUDIO_PANIC);
				break;
			}
			case WXK_LEFT:
			{
				meter->SetValue(meter->GetValue() - 1);
				break;
			}
			case WXK_DOWN:
			{
				if (organfile)
					organfile->GetFrameGeneral(meter->GetValue() - 1)->Push();
				break;
			}
			case WXK_RIGHT:
			{
				meter->SetValue(meter->GetValue() + 1);
				break;
			}
			default:
			{
				if (organfile && doc && doc->b_loaded && (k = WXKtoVK(k)))
				{
					for (int i = organfile->GetFirstManualIndex(); i <= organfile->GetManualAndPedalCount(); i++)
					{
						for (int j = 0; j < organfile->GetManual(i)->GetStopCount(); j++)
						{
							if (k == organfile->GetManual(i)->GetStop(j)->ShortcutKey)
								organfile->GetManual(i)->GetStop(j)->Push();
						}
						for (int j = 0; j < organfile->GetManual(i)->GetCouplerCount(); j++)
							if (k == organfile->GetManual(i)->GetCoupler(j)->ShortcutKey)
								organfile->GetManual(i)->GetCoupler(j)->Push();
						for (int j = 0; j < organfile->GetManual(i)->GetDivisionalCount(); j++)
							if (k == organfile->GetManual(i)->GetDivisional(j)->ShortcutKey)
								organfile->GetManual(i)->GetDivisional(j)->Push();
					}
					for (int j = 0; j < organfile->GetTremulantCount(); j++)
						if (k == organfile->GetTremulant(j)->ShortcutKey)
							organfile->GetTremulant(j)->Push();
					for (int j = 0; j < organfile->GetDivisionalCouplerCount(); j++)
						if (k == organfile->GetDivisionalCoupler(j)->ShortcutKey)
							organfile->GetDivisionalCoupler(j)->Push();
					for (int j = 0; j < organfile->GetGeneralCount(); j++)
						if (k == organfile->GetGeneral(j)->ShortcutKey)
							organfile->GetGeneral(j)->Push();
					for (int j = 0; j < organfile->GetNumberOfReversiblePistons(); j++)
						if (k == organfile->GetPiston(j)->ShortcutKey)
							organfile->GetPiston(j)->Push();
				}
				event.Skip();
			}
		}
	}
	event.Skip();
}

void OrganPanel::OnDraw(wxDC* dc)
{
	if (!m_clientBitmap.Ok() || !organfile || !displayMetrics->GetJambLeftRightWidth())
		return;
	dc->DrawBitmap(m_clientBitmap, m_clientOrigin.x, m_clientOrigin.y, false);
}
