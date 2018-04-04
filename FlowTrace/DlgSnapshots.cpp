#include "stdafx.h"
#include "resource.h"
#include "DlgSnapshots.h"

DlgSnapshots::DlgSnapshots()
{
  m_snapshot = gArchive.getSNAPSHOT();
  m_NodeCount = gArchive.getCount();
}

LRESULT DlgSnapshots::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  m_btnAdd.Attach(GetDlgItem(IDC_BUTTON_ADD));
  m_btnOK.Attach(GetDlgItem(IDOK));
  m_lstSnapsots.Attach(GetDlgItem(IDC_LIST_SNAPSHOTS));
  m_editComment.Attach(GetDlgItem(IDC_EDIT_COMMENT));

  fillSnapShots();

  if (CanNotAddSnapshot())
    m_btnAdd.EnableWindow(FALSE);  

  CenterWindow(GetParent());
  return TRUE;
}

bool DlgSnapshots::CanNotAddSnapshot()
{
  return m_NodeCount == 0 || (m_snapshot.snapShots.size() > 0 && m_NodeCount == m_snapshot.snapShots[m_snapshot.snapShots.size() - 1].pos);
}

LRESULT DlgSnapshots::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  Close(wID);
  return 0;
}

void DlgSnapshots::Close(WORD wID)
{
  if (wID == IDOK)
  {
    m_snapshot.curSnapShot = m_lstSnapsots.GetCurSel();
    UpdateSnapshots();
  }
  EndDialog(wID);
}


void DlgSnapshots::fillSnapShots()
{
  while (LB_ERR != m_lstSnapsots.DeleteString(0));
  m_lstSnapsots.AddString(_T("Show all"));
  if (m_snapshot.snapShots.size())
    m_lstSnapsots.AddString(_T("Show after last gArchive.getSNAPSHOT()"));

  CString str;
  size_t c = m_snapshot.snapShots.size();
  for (size_t i = 0; i < c; i++)
  {
    str.Format(_T("%d-%d:  %s"), i ? m_snapshot.snapShots[i - 1].pos : 0, m_snapshot.snapShots[i].pos, m_snapshot.snapShots[i].descr);
    m_lstSnapsots.AddString(str);
  }

  m_lstSnapsots.SetCurSel(m_snapshot.curSnapShot);
}

void DlgSnapshots::UpdateSnapshots()
{
  gArchive.getSNAPSHOT() = m_snapshot;
  gArchive.getSNAPSHOT().update();
}

bool DlgSnapshots::AddSnapshot(CHAR* descr, bool update)
{
  if (CanNotAddSnapshot())
    return false;

  SNAPSHOT_INFO si;
  si.pos = m_NodeCount;

  size_t cb = min(_tcslen(descr), _countof(si.descr) - 1);
  memcpy(si.descr, descr, cb);
  si.descr[cb] = 0;

  m_snapshot.curSnapShot = 1;
  m_snapshot.snapShots.push_back(si);

  if (update)
    UpdateSnapshots();

  return true;
}

LRESULT DlgSnapshots::OnBnClickedButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CHAR descr[32];
  int cb = sizeof(descr) / sizeof(descr[0]) - 1;
  m_editComment.GetWindowText(descr, cb);
  descr[cb] = 0;

  AddSnapshot(descr, false);

  m_btnAdd.EnableWindow(FALSE);

  fillSnapShots();

  return 0;
}

LRESULT DlgSnapshots::OnLbnDblClickedLstSnapshots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  if (m_lstSnapsots.GetCurSel() >= 0)
    Close(IDOK);
  return 0;
}
