#pragma once
#include "Archive.h"

extern ADDR_INFO* pInvalidAddrInfo;

class AddrInfo
{
public:
    static void Resolve(FLOW_NODE* flowNode);
    static void Reset();
};

