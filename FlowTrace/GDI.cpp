#include "stdafx.h"
#include "Settings.h"
#include "GDI.h"


HBRUSH Gdi::SelectionBrush = NULL;

void Gdi::ApplySettings(bool fontChanged)
{
	if (SelectionBrush)
		DeleteObject(SelectionBrush);
	SelectionBrush = ::CreateSolidBrush(gSettings.SelectionBkColor());
}


MemDC::MemDC()
{
	ZeroMemory(this, sizeof(*this));
	CreateCompatibleDC(NULL);
	m_bkBrush = ::CreateSolidBrush(m_bkColor);
}

MemDC::~MemDC()
{
	if (m_hBitmap)
		DeleteObject(m_hBitmap);
	if (m_bkBrush)
		DeleteObject(m_bkBrush);
}

void MemDC::CreateCompatibleBitmap(HDC hdc, int cx, int cy)
{
	ATLASSERT(cx && cy);
	if (m_cx != cx || m_cy != cy)
	{
		if (m_hBitmap)
			DeleteObject(m_hBitmap);
		m_cx = cx;
		m_cy = cy;
		m_hBitmap = ::CreateCompatibleBitmap(hdc, cx, cy);
		SelectBitmap(m_hBitmap);
	}
}

void MemDC::SetBkColor(DWORD bkColor)
{
	if (bkColor != m_bkColor)
	{
		if (m_bkBrush)
			DeleteObject(m_bkBrush);
		m_bkColor = bkColor;
		m_bkBrush = ::CreateSolidBrush(bkColor);
	}
}

BOOL MemDC::EraseBackground()
{
	RECT rc = { 0, 0, m_cx, m_cy };
	return FillRect(&rc, m_bkBrush);
}
