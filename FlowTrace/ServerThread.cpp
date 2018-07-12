#include "stdafx.h"
#include "ServerThread.h"
#include "Archive.h"
#include "Helpers.h"
#include "Settings.h"

ServerThread::ServerThread()
{
  struct sockaddr_in server;

  //Create a socket
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
  {
    Helpers::SysErrMessageBox(TEXT("Can not create socket"));
    goto err;
  }

  int opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
  {
    Helpers::SysErrMessageBox(TEXT("setsockopt for SOL_SOCKET failed\n"));
    goto err;
  }

  //Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  //server.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  server.sin_port = htons((USHORT)gSettings.GetUdpPort());

  //Bind
  if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
  {
    Helpers::SysErrMessageBox(TEXT("Bind failed : %d"));
    goto err;
  }
  return;
err:
  if (s != INVALID_SOCKET)
    closesocket(s);
  s = INVALID_SOCKET;
}


void ServerThread::Terminate()
{
  SOCKET s0 = s;
  s = INVALID_SOCKET;
  if (s0 != INVALID_SOCKET)
    closesocket(s0);
}

void ServerThread::Work(LPVOID pWorkParam)
{
  char buf[MAX_RECORD_LEN] = { 0 };
  int slen, cb, cb_read;
  struct sockaddr_in si_other;
  int64_t time, pc_time, rec_time, pac_time;
  DWORD pc_sec, pc_msec;

  if (s == INVALID_SOCKET)
    return;

  slen = sizeof(si_other);

  //keep listening for data
  while (true)
  {
    //try to receive data, this is a blocking call
    if ((cb = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen)) < 0)
    {
      break;
    }
    UDP_PACK* pack = (UDP_PACK*)buf;
    if (cb != pack->data_len + sizeof(UDP_PACK))
    {
      stdlog("incompleate package received %d %d\n", cb, pack->data_len + sizeof(UDP_PACK));
    }

    Helpers::GetTime(pc_sec, pc_msec);
    pc_time = (int64_t)pc_sec * (int64_t)1000 + (int64_t)pc_msec;
    pac_time = (int64_t)pack->term_sec * (int64_t)1000 + (int64_t)pack->term_msec;

    ROW_LOG_REC* rec = (ROW_LOG_REC*)(buf + sizeof(UDP_PACK));
    cb_read = sizeof(UDP_PACK);
    while (cb_read < cb)
    {
      ATLASSERT(rec->isValid()); 
      if (rec->len > (cb - cb_read) || !rec->isValid())
      {
        break;
      }
      rec_time = (int64_t)rec->sec * (int64_t)1000 + (int64_t)rec->msec;
      time = rec_time + (pc_time - pac_time);
      if (!gArchive.append(rec, (DWORD)(time / (int64_t)1000), (DWORD)(time % (int64_t)1000), &si_other))
      {
        break;
      }
      cb_read += rec->len;
      rec = (ROW_LOG_REC*)(buf + cb_read);
    }
  }
  if (IsWorking())
    Helpers::SysErrMessageBox(TEXT("udp receive failed"));

  if (s != INVALID_SOCKET)
    closesocket(s);
  s = INVALID_SOCKET;
}