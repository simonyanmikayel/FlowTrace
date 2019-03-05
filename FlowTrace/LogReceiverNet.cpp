#include "stdafx.h"

#ifndef _USE_ADB
#include "LogReceiverNet.h"
#include "Helpers.h"
#include "Settings.h"

LogReceiverNet gLogReceiverNet;
LogReceiver* gpLogReceiver = &gLogReceiverNet;

static NetThread* pNetThreads[1024];
static int cNetThreads = 0;

void LogReceiverNet::start(bool reset)
{
	//logcat(0, 0);
	m_working = true;
	add(new UdpThread());
#ifdef USE_TCP
	add(new TcpListenThread());
#endif
}

void LogReceiverNet::stop()
{
	lock();
	m_working = false;
	for (int i = 0; i < cNetThreads; i++)
	{
		pNetThreads[i]->StopWork();
		delete pNetThreads[i];
	}
	cNetThreads = 0;
	unlock();
}

void LogReceiverNet::add(NetThread* pNetThread)
{
	lock();
	if (m_working && (cNetThreads < _countof(pNetThreads)))
	{
		pNetThreads[cNetThreads++] = pNetThread;
		pNetThread->StartWork();
		pNetThread->SetThreadPriority(THREAD_PRIORITY_HIGHEST);
	}
	else
	{
		delete pNetThread;
	}
	unlock();
}

void NetThread::Terminate()
{
	SOCKET s0 = s;
	s = INVALID_SOCKET;
	if (s0 != INVALID_SOCKET)
		closesocket(s0);
}

#ifdef USE_TCP
TcpReceiveThread::TcpReceiveThread(SOCKET clientSocket)
{
	s = clientSocket;
}

void TcpReceiveThread::Work(LPVOID pWorkParam)
{
	int slen, cb, cb_read;
	struct sockaddr_in si_other;
	bool packError = false;
	slen = sizeof(si_other);
	//TODO resolve si_other
	//keep listening for data
	while (true)
	{
		//try to receive data, this is a blocking call
		if ((cb = recv(s, buf, sizeof(NET_PACK_INFO), 0)) != sizeof(NET_PACK_INFO))
		{
			break;
		}
		NET_PACK_INFO* pack = (NET_PACK_INFO*)buf;
		if (pack->data_len <= 0 || pack->data_len > MAX_NET_BUF - sizeof(NET_PACK_INFO))
		{
			break;
		}
		cb_read = sizeof(NET_PACK_INFO);
		while (cb_read < pack->data_len + sizeof(NET_PACK_INFO)) {
			cb = recv(s, buf + cb_read, pack->data_len - cb_read + sizeof(NET_PACK_INFO), 0);
			if (cb <= 0) {
				break;
			}
			cb_read += cb;
		}
		if (cb_read != pack->data_len + sizeof(NET_PACK_INFO))
			break;

		cb = pack->data_len + sizeof(NET_PACK_INFO);
		cb_read = sizeof(NET_PACK_INFO);
		ROW_LOG_REC* rec = rec = (ROW_LOG_REC*)(buf + cb_read);
		gLogReceiverNet.lock();
		while (cb_read < cb)
		{
			ATLASSERT(rec->isValid());
			if (rec->len > (cb - cb_read) || !rec->isValid())
			{
				packError = true;
				break;
			}
			if (!gArchive.append(rec, NULL))//&si_other
			{
				packError = true;
				gLogReceiverNet.unlock();
				break;
			}
			cb_read += rec->len;
			rec = (ROW_LOG_REC*)(buf + cb_read);
		}
		gLogReceiverNet.unlock();
		if (cb_read != pack->data_len + sizeof(NET_PACK_INFO)) {
			packError = true;
			break;
		}
	}
	if (IsWorking() && packError)
		Helpers::SysErrMessageBox(TEXT("Tcp receive failed\nClear log to restart"));

	Terminate();
}

TcpListenThread::TcpListenThread()
{
	int iResult;
	s = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	char listenPort[32];
	_itoa_s(gSettings.GetUdpPort(), listenPort, 10);
	// Resolve the server address and port
	iResult = getaddrinfo(NULL, listenPort, &hints, &result);
	if (iResult != 0) {
		Helpers::ErrMessageBox(TEXT("getaddrinfo failed with error: %d"), iResult);
		goto err;
	}

	// Create a SOCKET for connecting to server
	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s == INVALID_SOCKET) {
		Helpers::ErrMessageBox(TEXT("socket failed with error: %ld"), WSAGetLastError());
		goto err;
	}

	// Setup the TCP listening socket
	iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		Helpers::ErrMessageBox(TEXT("TCT socket bind failed with error: %ld"), WSAGetLastError());
		goto err;
	}

	freeaddrinfo(result);
	return;

err:
	if (result)
		freeaddrinfo(result);
	Terminate();
}

void TcpListenThread::Work(LPVOID pWorkParam)
{
	if (s == INVALID_SOCKET)
		return;
	if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
		Helpers::ErrMessageBox(TEXT("listen failed with error: %d\nClear log to restart"), WSAGetLastError());
		Terminate();
		return;
	}
	//keep listening for connection
	while (gLogReceiverNet.working())
	{
		// Accept a client socket
		SOCKET clientSocket = accept(s, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			int err = WSAGetLastError();
			if (gLogReceiverNet.working())
				Helpers::ErrMessageBox(TEXT("accept failed with error: %d\nClear log to restart"), WSAGetLastError());
			Terminate();
			return;
		}
		TcpReceiveThread* pTcpReceiveThread = new TcpReceiveThread(clientSocket);
		gLogReceiverNet.add(pTcpReceiveThread);
	}
}
#endif //USE_TCP

UdpThread::UdpThread()
{
	struct sockaddr_in server;

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		Helpers::SysErrMessageBox(TEXT("Can not create socket\nClear log to restart"));
		goto err;
	}

	int opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
	{
		Helpers::SysErrMessageBox(TEXT("setsockopt for SOL_SOCKET failed\nClear log to restart"));
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
		Helpers::SysErrMessageBox(TEXT("UDP socket bind failed\nClear log to restart"));
		goto err;
	}
	return;

err:
	Terminate();
}

void UdpThread::Work(LPVOID pWorkParam)
{
	int slen, cb, cb_read;
	struct sockaddr_in si_other;
	slen = sizeof(si_other);
	NET_PACK_INFO* pack = (NET_PACK_INFO*)buf;

	int ack_retry_count = 0;
	DWORD dwLastTimeout = INFINITE;
	DWORD dwAckTimeout = INFINITE;
	//keep listening for data
	while (true)
	{
		DWORD dwTimeout = ack_retry_count ? 20 : INFINITE;
		if (dwLastTimeout != dwTimeout) {
			setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeout, sizeof(DWORD));
			dwLastTimeout = dwTimeout;
		}

		//try to receive data, this is a blocking call
		if ((cb = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen)) < 0)
		{
			if (ack_retry_count) {
				if (pack->pack_nn < 0)
					pack->pack_nn = -pack->pack_nn;
				//stdlog("retry ack %d\n", pack->pack_nn);
				sendto(s, (const char*)pack, sizeof(NET_PACK_INFO), 0, (struct sockaddr *) &si_other, sizeof(si_other));
				ack_retry_count--;
				continue;
			}
			break;
		}
		if (cb != pack->data_len + sizeof(NET_PACK_INFO))
		{
			stdlog("incompleate package received %d %d\n", cb, pack->data_len + sizeof(NET_PACK_INFO));
			continue;
		}
		if (pack->pack_nn && pack->retry_count)
		{
			if ((sendto(s, (const char*)pack, sizeof(NET_PACK_INFO), 0, (struct sockaddr *) &si_other, sizeof(si_other))) < 0)
			{
				break;
			}
			//stdlog("received pack %d\n", pack->pack_nn);
			ack_retry_count = pack->retry_count - 1;
			dwAckTimeout = pack->retry_delay / 1000;
		}
		if (pack->data_len == 0)
			continue;

		ROW_LOG_REC* rec = (ROW_LOG_REC*)(buf + sizeof(NET_PACK_INFO));
		cb_read = sizeof(NET_PACK_INFO);
		gLogReceiverNet.lock();
		while (cb_read < cb)
		{
			ATLASSERT(rec->isValid());
			if (rec->len > (cb - cb_read) || !rec->isValid())
			{
				//Terminate();
				break;
			}
			if (!gArchive.append(rec, &si_other, false, 0, pack))
			{
				break;
			}
			cb_read += rec->len;
			rec = (ROW_LOG_REC*)(buf + cb_read);
		}
		gLogReceiverNet.unlock();
	}

	if (IsWorking())
		Helpers::SysErrMessageBox(TEXT("Udp receive failed\nClear log to restart"));

	Terminate();
}
#endif //_USE_ADB