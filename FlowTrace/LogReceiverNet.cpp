#include "stdafx.h"

#include "LogReceiverNet.h"
#include "Helpers.h"
#include "Settings.h"

LogReceiverNet gLogReceiverNet;

void LogReceiverNet::start()
{
	//logcat(0, 0);
#ifdef USE_UDP
	add(new UdpThread());
#endif
#ifdef USE_TCP
	add(new TcpListenThread());
#endif
#ifdef USE_RAW_TCP
	if (gSettings.GetRawTcpPort())
		add(new RawTcpListenThread());
#endif
}

void LogReceiverNet::stop()
{
	for (int i = 0; i < cThreads; i++)
	{
		pThreads[i]->StopWork();
		delete pThreads[i];
	}
	cThreads = 0;
}

void LogReceiverNet::add(NetThread* pNetThread)
{
	if (gLogReceiver.working() && (cThreads < _countof(pThreads)))
	{
		pThreads[cThreads++] = pNetThread;
		pNetThread->StartWork();
	}
	else
	{
		delete pNetThread;
	}
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
	bool packError = false;
	slen = sizeof(si_other);
	SetThreadPriority(THREAD_PRIORITY_HIGHEST);
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
		gLogReceiver.lock();
		while (cb_read < cb)
		{
			ATLASSERT(rec->isValid());
			if (rec->len > (cb - cb_read) || !rec->isValid())
			{
				packError = true;
				break;
			}
			if (!gArchive.appendNet(rec, NULL))//&si_other
			{
				packError = true;
				gLogReceiver.unlock();
				break;
			}
			cb_read += rec->len;
			rec = (ROW_LOG_REC*)(buf + cb_read);
		}
		gLogReceiver.unlock();
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
	while (gLogReceiver.working())
	{
		// Accept a client socket
		SOCKET clientSocket = accept(s, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			int err = WSAGetLastError();
			if (gLogReceiver.working())
				Helpers::ErrMessageBox(TEXT("accept failed with error: %d\nClear log to restart"), WSAGetLastError());
			Terminate();
			return;
		}
		TcpReceiveThread* pTcpReceiveThread = new TcpReceiveThread(clientSocket);
		gLogReceiverNet.add(pTcpReceiveThread);
	}
}
#endif //USE_TCP

#ifdef USE_RAW_TCP
RawTcpReceiveThread::RawTcpReceiveThread(SOCKET clientSocket)
{
	s = clientSocket;
	logData.reset("<???>");
}

void RawTcpReceiveThread::Work(LPVOID pWorkParam)
{
	int len;
	SetThreadPriority(THREAD_PRIORITY_HIGHEST);
	//keep listening for data
	while (true)
	{
		//try to receive data, this is a blocking call
		if ((len = recv(s, buffer, sizeof(buffer) - 1, 0)) <= 0)
		{
			break;
		}
		buffer[len] = 0;
		char* szLog = buffer;
		int cbLog = len;
		logData.pid = -(gLogReceiverNet.threadCount() + 100);
		logData.tid = logData.pid;
		DWORD sec, msec;
		Helpers::GetTime(sec, msec);
		logData.sec = sec;
		logData.msec = msec;

		gLogReceiver.lock();
		logData.p_trace = szLog;
		logData.cb_trace = (WORD)cbLog;
		logData.len = sizeof(LOG_REC_BASE_DATA) + logData.cbData();
		logData.color = 0;
		gArchive.appendSerial(&logData);
		gLogReceiver.unlock();
	}

	Terminate();
}

RawTcpListenThread::RawTcpListenThread()
{
	int iResult;
	s = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	char listenPort[32];
	_itoa_s(gSettings.GetRawTcpPort(), listenPort, 10);
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

void RawTcpListenThread::Work(LPVOID pWorkParam)
{
	if (s == INVALID_SOCKET)
		return;
	if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
		Helpers::ErrMessageBox(TEXT("listen failed with error: %d\nClear log to restart"), WSAGetLastError());
		Terminate();
		return;
	}
	//keep listening for connection
	while (gLogReceiver.working())
	{
		// Accept a client socket
		SOCKET clientSocket = accept(s, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			int err = WSAGetLastError();
			if (gLogReceiver.working())
				Helpers::ErrMessageBox(TEXT("accept failed with error: %d\nClear log to restart"), WSAGetLastError());
			Terminate();
			return;
		}
		RawTcpReceiveThread* pRawTcpReceiveThread = new RawTcpReceiveThread(clientSocket);
		gLogReceiverNet.add(pRawTcpReceiveThread);
	}
}
#endif //USE_RAW_TCP

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

	//int           iSize, iVal, ret;
	//iSize = sizeof(iVal);
	//ret = getsockopt(s, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&iVal, &iSize);
	//if (ret == SOCKET_ERROR)
	//{
	//	int err = WSAGetLastError();
	//}

	//int const buff_size = 1024 * 1024;
	//if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const *>(&buff_size), sizeof(buff_size)) < 0)
	//{
	//	int err = WSAGetLastError();
	//}

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
#ifdef  USE_RECORDER_THREAD
	recorder.StartWork();
#endif
	return;

err:
	Terminate();
}
bool Unreachable() {
	//On a UDP - datagram socket this error indicates a previous send operation resulted in an ICMP Port Unreachable message.
	int err = WSAGetLastError();
	return (err == WSAECONNRESET);
}

//#define _USE_RETRY_COUNT
void UdpThread::Work(LPVOID pWorkParam)
{
	int slen, cb_recv;
	slen = sizeof(sockaddr_in);

	SetThreadPriority(THREAD_PRIORITY_HIGHEST);

	int ack_retry_count = 0;
#ifdef _USE_RETRY_COUNT
	DWORD dwLastTimeout = INFINITE;
	DWORD dwAckTimeout = INFINITE;
#endif //_USE_RETRY_COUNT
	//keep listening for data
	while (true)
	{
#ifdef _USE_RETRY_COUNT
		DWORD dwTimeout = ack_retry_count > 0 ? dwAckTimeout : INFINITE;
		if (dwLastTimeout != dwTimeout) {
			setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeout, sizeof(DWORD));
			dwLastTimeout = dwTimeout;
		}
#endif //_USE_RETRY_COUNT

#ifdef  USE_RECORDER_THREAD
		NET_PACK* pack = recorder.GetNetPak();
#else
		NET_PACK* pack = &thePack;
#endif
		if (!pack)
			break;
		//try to receive data, this is a blocking call
		if ((cb_recv = recvfrom(s, (char*)pack, NET_PACK::PackSize(), 0, (struct sockaddr *) &pack->si_other, &slen)) < 4)
		{
#ifdef _USE_RETRY_COUNT
			if (ack_retry_count) {
				//stdlog("retry ack %d\n", pack->pack_nn);
				sendto(s, (const char*)pack, sizeof(NET_PACK_INFO), 0, (struct sockaddr *) &si_other, sizeof(si_other));
				ack_retry_count--;
				SetPackState(pack, NET_PACK_FREE);
				continue;
			}
#endif //_USE_RETRY_COUNT
			if (Unreachable()) {
				ack_retry_count = 0;
				SetPackState(pack, NET_PACK_FREE);
				continue;
			}
			else {
				break;
			}
		}
		if (cb_recv != pack->info.data_len + sizeof(NET_PACK_INFO))
		{
			stdlog("incompleate package received %d %d\n", cb_recv, pack->info.data_len + sizeof(NET_PACK_INFO));
			SetPackState(pack, NET_PACK_FREE);
			continue;
		}
		if (!pack->info.retry_count)
			pack->info.pack_nn = 0;//turn of packet checking
		if (pack->info.pack_nn)
		{
			if ((sendto(s, (const char*)pack, sizeof(NET_PACK_INFO), 0, (struct sockaddr *) &pack->si_other, sizeof(sockaddr_in))) < 0)
			{
				if (Unreachable()) {
					ack_retry_count = 0;
					SetPackState(pack, NET_PACK_FREE);
					continue;
				}
				else 
				{
					break;
				}
			}
			//stdlog("received pack %d\n", pack->pack_nn);
#ifdef _USE_RETRY_COUNT
			ack_retry_count = pack->retry_count - 1;
			dwAckTimeout = pack->retry_delay / 1000;
#endif //_USE_RETRY_COUNT
		}
		if (pack->info.data_len == 0)
		{
			SetPackState(pack, NET_PACK_FREE);
			continue; //ping packet
		}

		SetPackState(pack, NET_PACK_READY);
#ifdef  USE_RECORDER_THREAD
		recorder.ResumeThread();
#else
		gLogReceiver.lock();
		bool ss = RecordNetPack(pack);
		gLogReceiver.unlock();
		if (!ss)
			break;
#endif
	}

	if (IsWorking())
		Helpers::SysErrMessageBox(TEXT("Udp receive failed\nClear log to restart"));

	Terminate();
}
