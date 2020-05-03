#pragma once
#include "Archive.h"

extern ADDR_INFO* pInvalidAddrInfo;

class AddrInfo
{
public:
    static void Resolve(INFO_NODE* infoNode);
    static void Reset();
};

