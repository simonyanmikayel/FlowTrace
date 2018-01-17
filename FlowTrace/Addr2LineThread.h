#pragma once
#include "WorkerThread.h"
#include "LogData.h"

class Addr2LineThread :
  public WorkerThread
{
public:
  Addr2LineThread();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);
  void Resolve();

private:
  bool connectToServer(char *szServerName, WORD portNum);
  DWORD readUrl2(APP_DATA* appData);
  void CloseSocket();
  ADDR_INFO* last_info;
  SOCKET conn;
  HANDLE m_hWorkEvent;
};
