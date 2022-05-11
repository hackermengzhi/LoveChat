#include<stdio.h>
#include<WinSock2.h>
#include<string>
#include<conio.h>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2022
const char* errorReturn[4] = {"初始化失败！","连接服务器失败，请检查网络连接！",
	"接收信息失败,或许是服务器寄了","未知错误"
};

SOCKET serverSocket; //网络套接字
sockaddr_in sockAddr;
char nickName[16];
char line1[111];//divide line
char line2[111];//empty string
HANDLE hMutex;//互斥锁
bool init() {
	//WinSocket Initial
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1, 1), &data);
	if (ret != 0) {
		return false;
	}
	//WinSocket套接字
	socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//if (ret != 0)
	//	return false;//unsure
	//物理地址
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
	sockAddr.sin_port = htons(SERVER_PORT);

	hMutex =CreateMutex(0, 0, "console");
	return true;
}
void GBKToUTF8(string& strGBK) {
	int len = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
	wchar_t* wszUtf8 = new wchar_t[len];
	memset(wszUtf8, 0, len);
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, wszUtf8, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wszUtf8, -1, NULL, 0, NULL, NULL);
	char* szUtf8 = new char[len + 1];
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wszUtf8, -1, szUtf8, len, NULL, NULL);
	strGBK = szUtf8;
	delete[]szUtf8;
	delete[]wszUtf8;
}
void login() {
	system("mode con lines=5 cols=30\n");
	printf("	欢迎使用OICQ	\n\n");
	printf("		昵称：");
	scanf_s("%s", nickName, sizeof(nickName));

	while (getchar() != '\n');//清空输入缓冲区
	string name = nickName;
	GBKToUTF8(name);
	send(serverSocket, name.c_str(), name.size() + 1, 0);
	//类型检查没做
}
void gotoxy(int x, int y) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { x,y };
	SetConsoleCursorPosition(hOut, pos);
}
void uiInit() {
	system("mode con lines=36 cols=110");
	system("cls");
	gotoxy(0, 33);

	for (int i = 0; i < 110; i++) {
		line1[i] = '-';
		line2[i] = ' ';
	}
	line1[110] = line2[110] = 0;
	printf("%s", line1);
}
void printMsg(const char* msg) {
	//上锁,等待时间无限
	WaitForSingleObject(hMutex,INFINITE);
	//主功能
	static POINT pos = { 0,0 };//print position
	gotoxy(pos.x, pos.y);
	static int color = 31;
	printf("\033[0:%d40m%s\033[0m", color++,msg);
	if (color > 36) color = 31;
	printf("%s\n", msg);
	
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hout, &info);
	pos.x=info.dwCursorPosition.X;
	pos.y = info.dwCursorPosition.Y;
	if (pos.y >= 33) {
		printf("%s\n", line2);
		printf("\n\n");
		gotoxy(0, 33);
		printf("%s\n", line1);
		pos.y--;
	}
	gotoxy(1, 34);
	//释放锁
	ReleaseMutex(hMutex);
}
string UTF8ToGBK(const char* strUTF8) {
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	string strTemp(szGBK);
	if (wszGBK) delete[]wszGBK;
	if (szGBK) delete[]szGBK;
	return strTemp;
}
void editPrint(int col,char ch) {
	WaitForSingleObject(hMutex, INFINITE);

	gotoxy(col, 34);
	printf("%c", ch);
	
	ReleaseMutex(hMutex);
}
void editPrint(int col, const char* str) {
	WaitForSingleObject(hMutex, INFINITE);

	gotoxy(col, 34);
	printf("%s", str);

	ReleaseMutex(hMutex);
}
DWORD WINAPI threadFuncRecv(LPVOID pram) {
	//TODO
	char buff[4096];
	while (1) {
		int ret = recv(serverSocket, buff, sizeof(buff), 0);
		if (ret <= 0) {
			//editPrint(34,errorReturn[2]);
			break;
		}

		//打印消息
			//TODO
		
		printMsg(UTF8ToGBK(buff).c_str());
	}
	return NULL;
}
bool isCharacter(char str[], int index) {
	int i = 0;
	while (i < index) {
		if (str[i] > 0) {
			i++;
		}
		else i += 2;
	}
	if (i == index) {
		return false;
	}
	else return true;
}
int main() {
	if (!init()) {
		printf("初始化失败！\n");
		return -1;
	}
	//像服务器发起连接
	int ret = connect(serverSocket,
		(SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if (ret != 0) {
		printf("连接服务器失败，请检查网络连接！\n");
		//return -2;
	}

	//login
	login();
	uiInit();

	HANDLE hThread=CreateThread(0, 0, threadFuncRecv, 0, 0, 0);
	if(hThread)CloseHandle(hThread);
	

	while (1) {
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		editPrint(0,'>');
		int len = 0;
		while (1) {
			if (_kbhit()) {
				char c = getch();
				if (c == '\r') {
					break;
				}
				else if (c == 8) {
					if (len == 0) continue;
					if (isCharacter(buff, len - 1)) {
						//printf("\b\b \b\b");
						editPrint(len + 1, "\b\b \b\b");
						buff[len - 1] = buff[len - 2] = 0;
						len -= 2;
					}
					else {
						editPrint(len + 1, "\b \b");
						buff[len - 1] = 0;
						len -= 1;
					}
					continue;
				}
				WaitForSingleObject(hMutex, INFINITE);

				do {
					printf("%c", c);
					buff[len++] = c;

				} while (_kbhit() && (c=getch()));

				ReleaseMutex(hMutex);
				//editPrint(len + 1, c);
				//buff[len++] = c;
			}

		}
		if (len == 0) {
			continue;
		}
		//cleareditbox
		char buff2[1024];
		sprintf_s(buff2, sizeof(buff2),"%s\n", line2);
		editPrint(0, buff2);
		//printmsg
		sprintf_s(buff2,"【LocalHost】:%s",buff);
		//printMsg(buff2);
		printMsg(buff2);
		//sendmessage
		send(serverSocket, buff, strlen(buff) + 1, 0);
	}
	
	return 0;
}
