// TestMemBuf.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MemBuf.h"


int main()
{
    MEM_BUF buf(102400);
    PtrArray<char> arr(&buf);
    arr.Add("11111111112");
    char* p = arr.Get(0);
    return 0;
}

