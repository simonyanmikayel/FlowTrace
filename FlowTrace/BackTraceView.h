#pragma once

#include "Archive.h"
#include "Helpers.h"

class CFlowTraceView;

class CBackTraceView : public CWindowImpl< CBackTraceView, CEdit>
{
public:
  CBackTraceView(CFlowTraceView* pView);
  ~CBackTraceView();

  BEGIN_MSG_MAP(CBackTraceView)
  END_MSG_MAP()


private:
  CFlowTraceView* m_pView;
};
