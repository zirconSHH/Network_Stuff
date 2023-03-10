#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <fstream>

using namespace std;

int maxclient = 10;
int defaultport = 10240;
#define MAXBUFF 1024

string GetFileName(const string& path)
{
	string name;
	int tmp=path.rfind("/");
	if(tmp==path.npos)
		tmp = path.rfind("\\");
	name = path.substr(tmp + 1, path.length() - tmp);
	return name;
}

void SetIP(SOCKADDR_IN& addr, const char* ip)
{
	inet_pton(AF_INET, ip, &addr.sin_addr);
}

bool CheckFile(const string& filepath)
{
	fstream file_check(filepath);
	if (file_check.good())
	{
		file_check.close();
		return true;
	}
	file_check.close();
	return false;
}

int TransferFile_send(const char* filepath, const char* targetIP, const short& port = defaultport)
{
	//check file existence
	if (!CheckFile(filepath))
	{
		cout << "no such file" << endl;
		return 1;
	}

	//initiate WSA
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		cout << "WSA start failed" << endl;
		WSACleanup();
		return -1;
	}

	//create host socket
	SOCKET socket_host = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_host == INVALID_SOCKET)
	{
		cout << "invalid socket" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_host);
		return 2;
	}

	//create target addr;
	SOCKADDR_IN addr_target;
	addr_target.sin_family = AF_INET;
	addr_target.sin_port = htons(port);
	SetIP(addr_target, targetIP);
	
	//connect to target
	if (connect(socket_host, (SOCKADDR*)&addr_target, sizeof(addr_target)) == SOCKET_ERROR)
	{
		cout << "connect error" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_host);
		return 3;
	}

	char sendbuff[MAXBUFF] = {};
	//send file name
	string filename = GetFileName(filepath);
	for (int i = 0; i < filename.length(); i++)
	{
		sendbuff[i] = filename[i];
	}
	/*sendbuff[filename.length()] = '\0';*/
	if (send(socket_host, sendbuff, MAXBUFF, 0) == SOCKET_ERROR)
	{
		cout << "send error" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_host);
		return 4;
	}
	cout << "name sended:" << sendbuff << endl;

	//open file
	fstream file;
	file.open(filepath, ios::in | ios::binary);
	cout << filepath << endl;
	if (!file.is_open())
	{
		cout << "open file failed" << endl;
		WSACleanup();
		closesocket(socket_host);
		file.close();
		return 5;
	}

	//send file data
	while (!file.eof())
	{
		memset(sendbuff, 0, MAXBUFF);
		file.read(sendbuff, MAXBUFF - 1);
		int readlen = file.gcount();
		cout << readlen << " " << sendbuff << "|" << endl;
		send(socket_host, sendbuff, readlen, 0);
	}
	memset(sendbuff, 0, MAXBUFF);
	send(socket_host, sendbuff, MAXBUFF, 0);

	file.close();

	WSACleanup();
	closesocket(socket_host);
	return 0;
}

int TransferFile_receive(const char* path, const short& port = defaultport)
{
	//initiate WSA
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		cout << "WSA start failed" << endl;
		WSACleanup();
		return -1;
	}

	//create listen socket
	SOCKET socket_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_listen == INVALID_SOCKET)
	{
		cout << "invalid socket" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_listen);
		return 1;
	}

	//create host address
	SOCKADDR_IN addr_host;
	addr_host.sin_family = AF_INET;
	addr_host.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_host.sin_port = htons(port);
	//bind to socket
	if (bind(socket_listen, (SOCKADDR*)&addr_host, sizeof(addr_host)) == SOCKET_ERROR)
	{
		cout << "bind error" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_listen);
		return 2;
	}

	//listen
	if (listen(socket_listen, 1) == SOCKET_ERROR)
	{
		cout << "listen error" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_listen);
		return 3;
	}
	cout << "listening" << endl;

	//accept send
	SOCKADDR_IN addr_origin;
	int size_origin = sizeof(addr_origin);
	SOCKET socket_host = accept(socket_listen, (SOCKADDR*)&addr_origin, &size_origin);
	if (socket_host == INVALID_SOCKET)
	{
		cout << "invalid socket" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_host);
		closesocket(socket_listen);
		return 4;
	}
	cout << "accepted" << endl;

	char recvbuff[MAXBUFF];
	//receice file name
	if (recv(socket_host, recvbuff, MAXBUFF, 0) == SOCKET_ERROR)
	{
		cout << "receive error" << WSAGetLastError() << endl;
		WSACleanup();
		closesocket(socket_host);
		closesocket(socket_listen);
		return 5;
	}

	//write file
	char filepath[MAXBUFF] = {};
	sprintf_s(filepath, "%s%s", path, recvbuff);

	if (CheckFile(filepath))
	{
		cout << "same name file already exists" << endl;
	}
	cout << filepath << "|" << endl;
	fstream file;
	file.open(filepath, fstream::out | fstream::binary | fstream::trunc);
	if (!file.is_open())
	{
		cout << "open file failed" << endl;
		WSACleanup();
		closesocket(socket_host);
		closesocket(socket_listen);
		file.close();
		return 6;
	}

	//receive file data
	memset(recvbuff, 0, MAXBUFF);
	while (int len = recv(socket_host, recvbuff, MAXBUFF, 0))
	{
		if (len > 0 && len < MAXBUFF)
		{
			cout << " " << len << recvbuff << "|" << endl;
			file.write(recvbuff, len);
			memset(recvbuff, 0, MAXBUFF);
		}
		else if (len == MAXBUFF)
			break;
			
	}

	file.close();
	WSACleanup();
	closesocket(socket_host);
	closesocket(socket_listen);

	return 0;
}