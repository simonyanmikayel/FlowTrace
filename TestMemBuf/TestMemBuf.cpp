// TestMemBuf.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Buffer.h"
struct Info
{
	INT64 i;
	static int Compare(Info *p1, Info *p2) {
		return (p1->i < p2->i) ? -1 : ((p1->i == p2->i) ? 0 : 1);
	}
	bool les(const Info* p)
	{
		return i < p->i;
	}
};
///////////////////////////////////////////////////////////////////
int main()
{ 
#ifdef WIN_64
    //MemBuf buf( 1024 * 1024 * 1024, 1024 * 1024); //8LL *
	MemBuf buf(1024 * 1024 * 1024, 1024);
#else
    //MemBuf buf(1024 * 1024 * 1024, 1024 * 1024);
	MemBuf buf(1024 * 1024, 1024);
#endif

	PtrArray<Info>* p = new PtrArray<Info>(&buf);
	INT64 c = 1024 *1024;
	bool sort = true;
    for (DWORD i = 0; i < c; i++)
    {
		Info* pi = p->AddR();
		if (!pi) {
			c = i;
			break;
		}
		pi->i = sort ? -(INT64(i)) : i;
		if (0 == i % (1024 * 1024))
			printf("add %d\n", i);
		
    }
	if (sort) {
		printf("sort\n");
		p->Sort();
	}
	for (DWORD i = 0; i < c; i++)
	{
		Info* pi = p->Get(i);
		if (sort)
			ATLASSERT(pi && pi->i == i - c + 1);
		else
			ATLASSERT(pi && pi->i == i);
		if (0 == i % (1024 * 1024))
			printf("get %d\n", i);

	}
	printf("Tot %llu mem %llu \n", (DWORD64)c, (DWORD64)buf.UsedMemory());
	buf.Free();
	_getch();
	return 0;
}
