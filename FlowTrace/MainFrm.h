// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Resource.h"
#include "Helpers.h"
#include "LogListView.h"
#include "LogTreeView.h"
#include "BackTraceView.h"
#include "FlowTraceView.h"

#define	TIMER_DATA_REFRESH 43
#define	TIMER_DATA_REFRESH_INTERVAL	500

class CMainFrame :
    public CFrameWindowImpl<CMainFrame>,
    public CUpdateUI<CMainFrame>,
    public CMessageFilter, public CIdleHandler
{
public:
    DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

    CMainFrame();
    CFlowTraceView m_view;
    CLogListView& m_list;
    CLogTreeView& m_tree;
    CBackTraceView& m_backTrace;
    CCommandBarCtrl m_CmdBar;

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnIdle();

    BEGIN_UPDATE_UI_MAP(CMainFrame)
        UPDATE_ELEMENT(ID_SEARCH_PREV, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_SEARCH_NEXT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_SEARCH_FIRST, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_SEARCH_LAST, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_SEARCH_REFRESH, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_SEARCH_CLEAR, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_EDIT_COPY, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_EDIT_SELECTALL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_SHOW_HIDE_TREE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_SHOW_HIDE_STACK, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_SHOW_HIDE_FLOW_TRACES, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_PAUSERECORDING, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_STARTRECORDIG, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_EDIT_FIND32798, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(CMainFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
        COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
        COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_VIEW_MODULES, OnViewModules)
		COMMAND_ID_HANDLER(ID_VIEW_FILTERS, OnViewFilters)
		COMMAND_ID_HANDLER(ID_VIEW_DETAILES, OnViewDetailes)
        COMMAND_ID_HANDLER(ID_VIEW_SETTINGS, OnViewSettings)
        COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
        COMMAND_ID_HANDLER(ID_FILE_EXPORT_TRACES, OnFileSave)
        COMMAND_ID_HANDLER(ID_FILE_IMPORT_TRACES, OnFileImport)
        COMMAND_ID_HANDLER(ID_FILE_IMPORT_LOGCAT, OnFileImportLogcat)
        COMMAND_ID_HANDLER(ID_VIEW_SHOW_HIDE_TREE, OnShowHideTreeView)
        COMMAND_ID_HANDLER(ID_VIEW_SHOW_HIDE_STACK, OnShowHideStack)
		COMMAND_ID_HANDLER(ID_VIEW_RUN_EXTERNAL_CMD_1, OnRunExternalCmd)
		COMMAND_ID_HANDLER(ID_VIEW_RUN_EXTERNAL_CMD_2, OnRunExternalCmd)
		COMMAND_ID_HANDLER(ID_VIEW_RUN_EXTERNAL_CMD_3, OnRunExternalCmd)
		COMMAND_ID_HANDLER(ID_VIEW_SHOW_HIDE_FLOW_TRACES, OnShowHideFlowTraces)
        COMMAND_ID_HANDLER(ID_VIEW_PAUSERECORDING, OnPauseRecording)
        COMMAND_ID_HANDLER(ID_VIEW_STARTRECORDIG, OnStartRecording)
        COMMAND_ID_HANDLER(ID_VIEW_CLEARLOG, OnClearLog)
		COMMAND_ID_HANDLER(ID_VIEW_RESETLOG, OnResetLog)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, onCopy)
        COMMAND_ID_HANDLER(ID_EDIT_SELECTALL, onSelectAll)
        MESSAGE_HANDLER(WM_INPORT_TASK, OnImportTask);
        MESSAGE_HANDLER(WM_UPDATE_FILTER, onUpdateFilter)
        MESSAGE_HANDLER(WM_UPDATE_TREE, onUpdateTree)
        MESSAGE_HANDLER(WM_SHOW_NGS, onShowMsg)
        MESSAGE_HANDLER(WM_UPDATE_BACK_TRACE, onUpdateBackTrace)

        COMMAND_ID_HANDLER(ID_BOOKMARKS_NEXTBOOKMARK, OnBookmarks)
        COMMAND_ID_HANDLER(ID_BOOKMARKS_PREVIOUSBOOKMARK, OnBookmarks)
        COMMAND_ID_HANDLER(ID_BOOKMARKS_CLEARBOOKMARKS, OnBookmarks)
        COMMAND_ID_HANDLER(ID_BOOKMARKS_TOGGLE, OnBookmarks)

        COMMAND_ID_HANDLER(ID_SEARCH_PREV, OnSearchNavigate)
        COMMAND_ID_HANDLER(ID_SEARCH_NEXT, OnSearchNavigate)
        COMMAND_ID_HANDLER(ID_SEARCH_FIRST, OnSearchNavigate)
        COMMAND_ID_HANDLER(ID_SEARCH_LAST, OnSearchNavigate)
        COMMAND_ID_HANDLER(ID_SEARCH_REFRESH, OnSearchRefresh)
        COMMAND_ID_HANDLER(ID_SEARCH_REFRESH_ON_EMTER, OnSearchRefreshOnEnter)
        COMMAND_ID_HANDLER(ID_SEARCH_CLEAR, OnSearchClear)
        COMMAND_ID_HANDLER(ID_SEARCH_MATCH_CASE, OnSearchSettings)
        COMMAND_ID_HANDLER(ID_SEARCH_MATCH_WHOLE_WORD, OnSearchSettings)
        COMMAND_ID_HANDLER(ID_SYNC_VIEWES, OnSyncViews)
        COMMAND_ID_HANDLER(ID_EDIT_FIND32798, OnEditFind)
		COMMAND_ID_HANDLER(ID_TREE_SHOW_THIS_APP, OnShowOnlyThisApp)
		COMMAND_ID_HANDLER(ID_TREE_SHOW_THIS_THREAD, OnShowOnlyThisThread)
		COMMAND_ID_HANDLER(ID_TREE_SHOW_ALL, OnShowAllApps)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
			 



    CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()

    // Handler prototypes (uncomment arguments if needed):
    //	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    //	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    //	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewModules(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewFilters(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewDetailes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFileImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFileImportLogcat(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnShowHideTreeView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnShowHideStack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRunExternalCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowHideFlowTraces(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnPauseRecording(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnStartRecording(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnClearLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnResetLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT onSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnImportTask(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onUpdateFilter(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onUpdateTree(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onShowMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onUpdateBackTrace(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSyncViews(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnTakeSnamshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnStartNewSnamshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBookmarks(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowOnlyThisApp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowOnlyThisThread(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowAllApps(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


    LRESULT OnSearchNavigate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSearchSettings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSearchClear(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSearchRefresh(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSearchRefreshOnEnter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


    HWND getSearchEdotBox() { return m_searchedit.m_hWnd; }
    void UpdateStatusBar();
    void SyncViews();
	void RedrawViews();
    void SetTitle();
    void LoadSearchList();
    void SaveSearchList();
    void FilterNode(WORD wID);
private:
    //Search support
    void SearchNavigate(WORD wID);
    CToolBarCtrl m_searchbar;
    CComboBoxEx m_cb;
    CEdit             m_searchResult;
    CComboBox           m_searchbox;
    CEdit               m_searchedit;
    HICON            m_lostIcon;
    CString          m_importExportFile;

    void UpdateSearchResult();
    void StartLogging(bool reset);
    void ClearLog(bool bRestart, bool reset);
    void StopLogging(bool bClearArcive, bool closing = false);
    void SearchRefresh(bool reset);
    void RefreshLog(bool resetSearch);

};

extern CMainFrame* gMainFrame;

