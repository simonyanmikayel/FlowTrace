#pragma once

#include "Archive.h"

class DlgSnapshots : public CDialogImpl<DlgSnapshots>
{
public:
  DlgSnapshots();

  enum { IDD = IDD_SNAPSHOTS };

  BEGIN_MSG_MAP(DlgSnapshots)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    COMMAND_HANDLER(IDC_BUTTON_ADD, BN_CLICKED, OnBnClickedButtonAdd)
    COMMAND_HANDLER(IDC_LIST_SNAPSHOTS, LBN_DBLCLK, OnLbnDblClickedLstSnapshots)
  END_MSG_MAP()

  CButton m_btnAdd;
  CButton m_btnOK;
  CListBox m_lstSnapsots;
  CEdit    m_editComment;

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnLbnDblClickedLstSnapshots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  void    Close(WORD wID);
  bool    CanNotAddSnapshot();
  bool    AddSnapshot(CHAR* descr, bool update);
  void    UpdateSnapshots();

  SNAPSHOT m_snapshot;
  int m_NodeCount;

  void fillSnapShots();
};
