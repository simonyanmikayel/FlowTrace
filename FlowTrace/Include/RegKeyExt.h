#pragma once

class CRegKeyExt :public CRegKeyEx
{
public:
  CRegKeyExt(LPCTSTR lpszKeyName, HKEY hKeyParent = HKEY_CURRENT_USER)
  {
    Create(hKeyParent, lpszKeyName);
  }

  //DWORD
  bool Write(LPCTSTR pszValueName, DWORD val)
  {
    return ERROR_SUCCESS == SetDWORDValue(pszValueName, val);
  }
  bool Read(LPCTSTR pszValueName, DWORD& val, DWORD defVal = 0)
  {
    bool bRet = (ERROR_SUCCESS == QueryDWORDValue(pszValueName, val));
    if (!bRet)
      val = defVal;
    return bRet;
  }

  //ULONGLONG
  bool Write(LPCTSTR pszValueName, ULONGLONG val)
  {
    return ERROR_SUCCESS == SetQWORDValue(pszValueName, val);
  }
  bool Read(LPCTSTR pszValueName, ULONGLONG& val, ULONGLONG defVal = 0)
  {
    bool bRet = (ERROR_SUCCESS == QueryQWORDValue(pszValueName, val));
    if (!bRet)
      val = defVal;
    return bRet;
  }

  //LONG
  bool Write(LPCTSTR pszValueName, LONG val)
  {
    return ERROR_SUCCESS == Write(pszValueName, (DWORD)val);
  }
  bool Read(LPCTSTR pszValueName, LONG& val, WORD defVal = 0)
  {
    DWORD dw = (LONG)val;
    bool bRet = Read(pszValueName, dw, (DWORD)defVal);
    val = (LONG)dw;
    return bRet;
  }

  //int
  bool Write(LPCTSTR pszValueName, int iValue)
  {
    return ERROR_SUCCESS == Write(pszValueName, (DWORD)iValue);
  }
  bool Read(LPCTSTR pszValueName, int& iValue, int defVal = 0)
  {
    DWORD dw = (DWORD)iValue;
    bool bRet = Read(pszValueName, dw, (DWORD)defVal);
    iValue = (int)dw;
    return bRet;
  }

  //WORD
  bool Write(LPCTSTR pszValueName, WORD val)
  {
    return ERROR_SUCCESS == Write(pszValueName, (DWORD)val);
  }
  bool Read(LPCTSTR pszValueName, WORD& val, WORD defVal = 0)
  {
    DWORD dw = (WORD)val;
    bool bRet = Read(pszValueName, dw, (DWORD)defVal);
    val = (WORD)dw;
    return bRet;
  }

  //BYTE
  bool Write(LPCTSTR pszValueName, BYTE val)
  {
    return ERROR_SUCCESS == Write(pszValueName, (DWORD)val);
  }
  bool Read(LPCTSTR pszValueName, BYTE& val, BYTE defVal = 0)
  {
    DWORD dw = (BYTE)val;
    bool bRet = Read(pszValueName, dw, (DWORD)defVal);
    val = (BYTE)dw;
    return bRet;
  }

  //LPTSTR
  bool Write(LPCTSTR pszValueName, LPCTSTR val)
  {
    DWORD dwType = REG_SZ;
    return ERROR_SUCCESS == SetStringValue(pszValueName, val, dwType);
  }
  bool Read(LPCTSTR pszValueName, LPTSTR val, int cChars, LPTSTR defVal = 0)
  {
    int c = cChars; //
    ULONG* pnChars = (ULONG*)&cChars;
    bool bRet = ERROR_SUCCESS == QueryStringValue(pszValueName, val, pnChars);
    if (!bRet && defVal)
    {
#pragma warning(disable: 4996) // _tcsncpy depricated
      _tcsncpy(val, defVal, c);
      val[c - 1] = 0;
#pragma warning(default: 4996)
    }
    return bRet;
  }

  //void*
  bool Read(LPCTSTR pszValueName, void* pValue, ULONG nBytes, bool init = false)
  {
    ULONG nBytesRead = nBytes; // initialize for sake of Win98
    if (init)
      ZeroMemory(pValue, nBytes);
    return ERROR_SUCCESS == QueryBinaryValue(pszValueName, pValue, &nBytesRead) && nBytesRead == nBytes;
  }
  bool Write(LPCTSTR pszValueName, void* pValue, ULONG nBytes)
  {
    return ERROR_SUCCESS == SetBinaryValue(pszValueName, pValue, nBytes);
  }
};
