#pragma once
#include "WorkerThread.h"

class ServerThread :
  public WorkerThread
{
public:
  ServerThread();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);

private:
  SOCKET s;
};

