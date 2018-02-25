#pragma once


inline void* Malloc(size_t size, size_t minSize)
{
    minSize = min(size, minSize);
    size_t allocSize = size;
    void * buf = nullptr;
    while (allocSize >= minSize)
    {
        buf = malloc(allocSize);
        if (buf == nullptr)
            allocSize = (allocSize / 5) * 4 + 1;
        else
            break;
    }
    size = buf ? allocSize : 0;
    return buf;
}

class MemBuf
{
public:
    MemBuf(DWORD64 maxBufSize, DWORD allocSize)
    {
        ZeroMemory(this, sizeof(*this));
        m_allocSize = allocSize;
        InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
        size_t chankCount = (unsigned long)(maxBufSize / m_allocSize) + 1;
        m_MemBufChank = chankCount ? (MemBufChank*)calloc(chankCount, sizeof(MemBufChank)) : nullptr;
        m_chankCount = m_MemBufChank ? chankCount : 0;
        m_chankEnd = m_MemBufChank ? (m_MemBufChank + m_chankCount) : nullptr;
        m_curChank = m_MemBufChank;
        _Alloc(m_allocSize); //alloc firest chank. this will be freeed only by destructor
    }

    ~MemBuf()
    {
        if (m_MemBufChank)
        {
            Free();
            free(m_MemBufChank->buf);
        }
        DeleteCriticalSection(&m_cs);
    }

    void Free()
    {
        if (m_MemBufChank)
        {
            EnterCriticalSection(&m_cs);
            if (m_chankCount > 1)
            {
                MemBufChank * chank = m_MemBufChank + 1; //do not free first chank
                while (chank <= m_curChank && chank < m_chankEnd)
                {
                    free(chank->buf);
                    chank++;
                }
                ZeroMemory(m_MemBufChank + 1, (m_chankCount - 1) * sizeof(MemBufChank));
            }
            m_curChank = m_MemBufChank;
            m_curChank->used = 0;
            m_usedMem = m_curChank->size;
            LeaveCriticalSection(&m_cs);
        }
    }

    template <typename Type> Type* New(size_t count, bool zero)
    {
        return (Type*)Alloc(sizeof(Type)*count, zero);
    }

    void * Alloc(size_t size, bool zero)
    {
        void * buf = nullptr;
        EnterCriticalSection(&m_cs);
        if (m_curChank < m_chankEnd)
        {
            buf = _Alloc(size);
            if (buf == nullptr)
            {
                m_curChank++;
                if (m_curChank < m_chankEnd)
                {
                    buf = _Alloc(size);
                }
            }
        }
        LeaveCriticalSection(&m_cs);
        if (zero && buf)
            ZeroMemory(buf, size);
        return buf;
    }

    size_t UsedMemory() { return m_usedMem; }

private:
    void * _Alloc(size_t size)
    {
        if (m_curChank == m_chankEnd)
            return nullptr;

        if (m_curChank->buf == nullptr)
        {
            size_t allocSize = max(m_allocSize, size);
            if (m_curChank->buf = Malloc(allocSize, allocSize)) // alloc at least m_allocSize
            {
                m_curChank->size = allocSize;
                m_usedMem += allocSize;
            }
            else
            {
                m_chankEnd = m_curChank;
                return nullptr;
            }
        }

        if (size > (m_curChank->size - m_curChank->used))
        {
            return nullptr;
        }

        void * buf = (char*)m_curChank->buf + m_curChank->used;
        m_curChank->used += size;
        return buf;
    }

    struct MemBufChank
    {
        void * buf;
        size_t size;
        size_t used;
    };

    CRITICAL_SECTION m_cs;
    MemBufChank * m_MemBufChank;
    MemBufChank * m_chankEnd;
    MemBufChank * m_curChank;
    size_t m_chankCount;
    size_t m_allocSize;
    size_t m_usedMem;
};

template <typename Type> class PtrArray
{
public:
    PtrArray(MemBuf* pMemBuf)
    {
        ZeroMemory(this, sizeof(*this));
        m_pMemBuf = pMemBuf;
        m_pp1 = (void**)m_pMemBuf->New<void*>(256, false);
        m_c1 = m_pp1 ? 0 : 256;
        m_c2 = m_c3 = m_c4 = 256; //trigger allocation
    }

    const Type* AddPtr(const Type* p)
    {
        const Type* pRet = nullptr;

        while (m_c1 < 256)
        {
            if (m_c2 == 256)
            {
                m_pp2 = (void**)(m_pp1[m_c1] = m_pMemBuf->New<void*>(256, false));
                if (m_pp2 == nullptr)
                {
                    m_c1 = 256;
                    continue;
                }
                m_c2 = 0;
                m_c1++;
            }
            if (m_c3 == 256)
            {
                m_pp3 = (void**)(m_pp2[m_c2] = m_pMemBuf->New<void*>(256, false));
                if (m_pp3 == nullptr)
                {
                    m_c1 = 256;
                    continue;
                }
                m_c3 = 0;
                m_c2++;
            }
            if (m_c4 == 256)
            {
                m_pp4 = (void**)(m_pp3[m_c3] = m_pMemBuf->New<void*>(256, false));
                if (m_pp4 == nullptr)
                {
                    m_c1 = 256;
                    continue;
                }
                m_c4 = 0;
                m_c3++;
            }

            m_pp4[m_c4] = (void*)p;
            pRet = p;
            m_c4++;
            m_Count++;
            break;
        }
        return pRet;
    }

    Type* Add(DWORD size, bool zero)
    {
        Type* p = (Type*)m_pMemBuf->Alloc(size, zero);
        if (p == nullptr)
            return nullptr;
        return (Type*)AddPtr(p);
    }

    Type* Get(DWORD i)
    {
        if (i >= m_Count)
            return nullptr;

        void **pp1, **pp2, **pp3, **pp4;
        DWORD c1, c2, c3, c4;

        pp1 = m_pp1;
        c1 = (i & 0xFF000000) >> 24;
        pp2 = (void**)pp1[c1];
        c2 = (i & 0x00FF0000) >> 16;
        pp3 = (void**)pp2[c2];
        c3 = (i & 0x0000FF00) >> 8;
        pp4 = (void**)pp3[c3];
        c4 = (i & 0x000000FF);
        Type* p = (Type*)pp4[c4];
        return p;
    }

    unsigned long Count() { return m_Count; }
    MemBuf * m_pMemBuf;
    DWORD m_Count;
    int m_c1, m_c2, m_c3, m_c4;
    void **m_pp1, **m_pp2, **m_pp3, **m_pp4;
};
