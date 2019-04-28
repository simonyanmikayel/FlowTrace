#pragma once
struct Gdi
{
	static void ApplySettings(bool fontChanged);
	static HBRUSH SelectionBrush;
};

class MemDC : public CDC
{
public:
	MemDC();
	~MemDC();

	void CreateCompatibleBitmap(HDC hdc, int cx, int cy);
	void SetBkColor(DWORD bkColor);
	BOOL EraseBackground();

	int m_cx;
	int m_cy;
	HBITMAP m_hBitmap;
	DWORD m_bkColor;
	HBRUSH m_bkBrush;
};

