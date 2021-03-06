// HTTPServer.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
/*
* html directory :
* ../html
*   ├── index.html
*   ├── css
*   ├── js
*   ├── images
*   ├── 404.html
*   ├── 400.html
*   └── other
* config :
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <process.h>
#include <time.h>

#define BUF_SIZE 2048
#define BUF_SMALL 100
#define INDEX "index.html"
#define MAX_GAME_NUM 10
#define COOKIE_LEN 15
struct game {
	char player1Cookie[COOKIE_LEN];//玩家1的cookie
	char player2Cookie[COOKIE_LEN];//玩家2的cookie
	int chessboard[3][3];//0:无，1：player1的棋子，2：player2的棋子
	int playerNumber;//玩家数量：0,1,2
	int turn;//轮到哪个玩家：1:player1 2:player2
	int result;//比赛结果：0：无结果 1：player1胜利 2：player2胜利 3：平局
};

struct game gameList[MAX_GAME_NUM];
int gameNumber = 0;


unsigned WINAPI RequestHandler(void *arg);
void Get(SOCKET sock, char *ct, char *filename, int has_cookie, char *cookie);
void Post(SOCKET sock, char *req, int has_cookie, char *cookie, char x, char y);
char *ContentType(char *file);
void SendErrorMSG(SOCKET sock, char *errorNum);
void ErrorHandling(char *message);
//get a cookie
char* genRandomString(int length);
//game
void loginGame(char *playerCookie);
int isEqual(char *str1, char *str2, int length);
int judge(int gameIndex);//0:无结果 1：player1胜 2：player2胜 3：平局

int main(int argc, char *argv[]) {
	argc = 2;
	argv[1] = (char*)"9190";
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	HANDLE hThread;
	DWORD dwThreadID;
	int clntAdrSize;
	if (argc != 2) {
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling((char*)"WSAStartup() error!");
	}
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling((char*)"bind() error!");
	}
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling((char*)"listen error!");
	}
	while (1) {
		clntAdrSize = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR *)&clntAdr, &clntAdrSize);
		printf("Connection Request : %s:%d\n",
			inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
		hThread = (HANDLE)_beginthreadex(NULL, 0, RequestHandler, (void*)hClntSock, 0, (unsigned*)&dwThreadID);
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned WINAPI RequestHandler(void *arg){
	SOCKET hClntSock = (SOCKET)arg;
	char buf[BUF_SIZE];

	char method[BUF_SMALL];
	char ct[BUF_SMALL];
	char fileName[BUF_SMALL];

	char cookie[COOKIE_LEN];
	char x[2], y[2];
	int has_cookie = 0;
	recv(hClntSock, buf, BUF_SIZE, 0);
	printf("%s\n", buf);
	if (strstr(buf, "HTTP/") == NULL) {
		strcpy(fileName, (char*)"400.html");
		//SendErrorMSG(hClntSock, (char*)"400");
	}
	else {
		char *find = strstr(buf, "Cookie: SESSIONID=");
		if (find == NULL) {
			has_cookie = 0;
		}
		else {
			has_cookie = 1;
			int i = 0;
			for (i = 0; i < COOKIE_LEN; i++) {
				cookie[i] = (find + 18)[i];
			}

		}
		find = strstr(buf, "x=");
		if (find == NULL) {

		}
		else {
			int i = 0;
			for (i = 0; i < 1; i++) {
				x[i] = (find + 2)[i];
			}
		}

		find = strstr(buf, "y=");
		if (find == NULL) {

		}
		else {
			int i = 0;
			for (i = 0; i < 1; i++) {
				y[i] = (find + 2)[i];
			}
		}

		strcpy(method, strtok(buf, " /"));
		if (strcmp(method, "GET")) {
			//SendErrorMSG(hClntSock, (char*)"400");
			if (strcmp(method, "POST")) {
				strcpy(fileName, (char*)"400.html");
			}
			else {
				//调用下棋程序
				strcpy(fileName, strtok(NULL, " "));
				Post(hClntSock, fileName, has_cookie, cookie, x[0], y[0]);
				return 0;
			}
		} else {
			strcpy(fileName, strtok(NULL, " "));
			if (strcmp(fileName, (char*)"/") == 0) { //default site
				strcpy(fileName, (char*)"index.html");
			}
		}
	}
	strcpy(ct, ContentType(fileName)); 
	Get(hClntSock, ct, fileName,has_cookie, cookie);
	return 0; 
}

void Get(SOCKET sock, char *ct, char *fileName, int has_cookie, char *cookie) {
	//check file
	FILE * sendFile;
	char temp[BUF_SMALL];
	snprintf(temp, BUF_SMALL, "./%s", fileName);//相对路径
	sendFile = fopen(temp, "rb");  
	int num = 200;
	
	if (sendFile == NULL) {
		//SendErrorMSG(sock, (char*)"404");
		sendFile = fopen("./404.html", "rb");
		num = 404;
	}
	fseek(sendFile, 0L, SEEK_END);
	int fileSize = ftell(sendFile);
	char protocol[BUF_SMALL];
	snprintf(protocol, BUF_SMALL, "HTTP/1.0 %d OK\r\n", num);
	
	char servName[] = "Server:Linux Web Server \r\n";

	char tempCookie[BUF_SMALL];
	if (has_cookie == 1) {
		snprintf(tempCookie, BUF_SMALL, "Cookie: SESSIONID=%s\r\n", cookie);
	}
	else if (has_cookie == 0){
		snprintf(tempCookie, BUF_SMALL, "Set-Cookie: SESSIONID=%s;\r\n", genRandomString(COOKIE_LEN));
	}
	char cntLen[BUF_SMALL];
	snprintf(cntLen, BUF_SMALL, "Content-length:%d\r\n", fileSize);
	char cntType[BUF_SMALL];
	char buf[BUF_SIZE];
	sprintf(cntType, "Content-type:%s\r\n\r\n", ct);

	/*transfer header info*/
	send(sock, protocol, strlen(protocol), 0);
	send(sock, servName, strlen(servName), 0);
	send(sock, cntLen, strlen(cntLen), 0);
	send(sock, tempCookie, strlen(tempCookie), 0);
	send(sock, cntType, strlen(cntType), 0);
	fseek(sendFile, 0, SEEK_SET);
	while (fread(buf, 1, 1, sendFile) != 0) {
		send(sock, buf, 1, 0);
	}
	closesocket(sock);
}

void Post(SOCKET sock, char *req, int has_cookie, char *cookie, char x, char y) {
	if (!strcmp(req, "/login")) {
		//check file
		FILE * sendFile;
		char temp[BUF_SMALL];
		snprintf(temp, BUF_SMALL, "./%s", "game.html");//相对路径
		sendFile = fopen(temp, "rb");
		int num = 302;

		if (sendFile == NULL) {
			//SendErrorMSG(sock, (char*)"404");
			sendFile = fopen("./400.html", "rb");
			num = 400;
		}
		fseek(sendFile, 0L, SEEK_END);
		int fileSize = ftell(sendFile);
		char protocol[BUF_SMALL];
		if (num == 302) {
			snprintf(protocol, BUF_SMALL, "HTTP/1.0 %d Found\r\n", num);
		}
		else if (num == 400) {
			snprintf(protocol, BUF_SMALL, "HTTP/1.0 %d Bad Request\r\n", num);
		}

		char servName[] = "Server:Linux Web Server \r\n";

		char tempCookie[BUF_SMALL];
		if (has_cookie == 1) {
			snprintf(tempCookie, BUF_SMALL, "Cookie: SESSIONID=%s\r\n", cookie);
		}
		else if (has_cookie == 0) {
			snprintf(tempCookie, BUF_SMALL, "Set-Cookie: SESSIONID=%s;\r\n", genRandomString(COOKIE_LEN));
		}
		char cntLen[BUF_SMALL];
		snprintf(cntLen, BUF_SMALL, "Content-length:%d\r\n", fileSize);
		char cntType[BUF_SMALL];
		char buf[BUF_SIZE];
		sprintf(cntType, "Content-type:%s\r\n\r\n", "text/html");

		/*transfer header info*/
		send(sock, protocol, strlen(protocol), 0);
		send(sock, servName, strlen(servName), 0);
		send(sock, cntLen, strlen(cntLen), 0);
		send(sock, tempCookie, strlen(tempCookie), 0);
		send(sock, cntType, strlen(cntType), 0);
		fseek(sendFile, 0, SEEK_SET);
		while (fread(buf, 1, 1, sendFile) != 0) {
			send(sock, buf, 1, 0);
		}
		closesocket(sock);
		loginGame(cookie);
	}
	else if (!strcmp(req, "/next")) {
		//[{00,01,02},{10,11,12},{20,21,22}]
		char jsonData[BUF_SMALL];
		char protocol[] = "HTTP/1.0 200 OK\r\n";
		char servName[] = "Server: Linux Web Server \r\n";
		char cntLen[] = "Content-length: 26\r\n";
		char contentType[] = "Content-Type: application/json;charset=UTF-8\r\n\r\n";
		//根据cookie找到对应的局
		int currentGame = -1;
		int currentPlayer = -1;
		int i = 0;
		for (i = 0; i < MAX_GAME_NUM; i++) {
			//if (strcmp(gameList[i].player1Cookie, cookie) == 0) {
			//	currentGame = i;
			//	currentPlayer = 1;
			//}
			//else if (strcmp(gameList[i].player2Cookie, cookie) == 0) {
			//	currentGame = i;
			//	currentPlayer = 2;
			//}
			if (isEqual(gameList[i].player1Cookie, cookie, COOKIE_LEN)) {
				currentGame = i;
				currentPlayer = 1;
			}
			else if (isEqual(gameList[i].player2Cookie, cookie, COOKIE_LEN)) {
				currentGame = i;
				currentPlayer = 2;
			}
		}
		if (currentGame != -1) {

			if (gameList[currentGame].playerNumber == 2) {
				int current = gameList[currentGame].turn;
				if (current == currentPlayer) {
					int xPos = x - '0';
					int yPos = y - '0';
					if (gameList[currentGame].chessboard[xPos][yPos] == 0) {
						gameList[currentGame].chessboard[xPos][yPos] = currentPlayer;

						//判断胜负
						if (judge(currentGame) == 1) {
							if (currentPlayer == 1) {
								strcpy(jsonData, "{\"message\":\"You Win!\"}");
								gameList[currentGame].result = 1;
							}
							else {
								strcpy(jsonData, "{\"message\":\"You Lose...\"}");
								gameList[currentGame].result = 2;
							}
						}
						else if (judge(currentGame) == 2) {
							if (currentPlayer == 2) {
								strcpy(jsonData, "{\"message\":\"You Win!\"}");
								gameList[currentGame].result = 2;
							}
							else {
								strcpy(jsonData, "{\"message\":\"You Lose...\"}");
								gameList[currentGame].result = 1;
							}
						}
						else if (judge(currentGame) == 3) {
							strcpy(jsonData, "{\"message\":\"Ends in a draw\"}");
						}else {
							//发送棋盘json
							//json data
							snprintf(jsonData, BUF_SMALL,
								"[{\"00\":%d,\"01\":%d,\"02\":%d},"
								"{\"10\":%d,\"11\":%d,\"12\":%d},"
								"{\"20\":%d,\"21\":%d,\"22\":%d}]",
								gameList[currentGame].chessboard[0][0], gameList[currentGame].chessboard[0][1], gameList[currentGame].chessboard[0][2],
								gameList[currentGame].chessboard[1][0], gameList[currentGame].chessboard[1][1], gameList[currentGame].chessboard[1][2],
								gameList[currentGame].chessboard[2][0], gameList[currentGame].chessboard[2][1], gameList[currentGame].chessboard[2][2]);
							if (gameList[currentGame].turn == 1) {
								gameList[currentGame].turn = 2;
							}
							else if (gameList[currentGame].turn == 2) {
								gameList[currentGame].turn = 1;
							}
						}
					}
					else {
						//这个位置有棋子
						strcpy(jsonData, "{\"message\":\" There is a chessman in this position \"}");
					}
				}
				else {
					//没轮到当前用户
					strcpy(jsonData, "{\"message\":\" It's not your turn \"}");
				}
			}
			else {
				//只有一个用户
				strcpy(jsonData, "{\"message\":\" Please wait for the player to join the game \"}");
			}
			
		}

		printf("Json Data : ---- %s\n", jsonData);

		send(sock, protocol, strlen(protocol), 0);
		send(sock, servName, strlen(servName), 0);
		send(sock, contentType, strlen(contentType), 0);
		send(sock, jsonData, strlen(jsonData), 0);
		closesocket(sock);
	}
	else if (!strcmp(req, "/refresh")) {
		//[{00,01,02},{10,11,12},{20,21,22}]
		char jsonData[BUF_SMALL];
		char protocol[] = "HTTP/1.0 200 OK\r\n";
		char servName[] = "Server: Linux Web Server \r\n";
		char cntLen[] = "Content-length: 26\r\n";
		char contentType[] = "Content-Type: application/json;charset=UTF-8\r\n\r\n";
		//根据cookie找到对应的局
		int currentGame = -1;
		int currentPlayer = -1;
		int i = 0;
		for (i = 0; i < MAX_GAME_NUM; i++) {
			//if (strcmp(gameList[i].player1Cookie, cookie)) {
			//	currentGame = i;
			//	currentPlayer = 1;
			//}
			//else if (strcmp(gameList[i].player2Cookie, cookie)) {
			//	currentGame = i;
			//	currentPlayer = 2;
			//}
			if (isEqual(gameList[i].player1Cookie, cookie, COOKIE_LEN)) {
				currentGame = i;
				currentPlayer = 1;
			}
			else if (isEqual(gameList[i].player2Cookie, cookie, COOKIE_LEN)) {
				currentGame = i;
				currentPlayer = 2;
			}
		}
		if (currentGame != -1) {
			//判断胜负
			int result = gameList[currentGame].result;
			if ( result == 1) {
				if (currentPlayer == 1)
					strcpy(jsonData, "{\"message\":\"You Win!\"}");
				else
					strcpy(jsonData, "{\"message\":\"You Lose...\"}");
			}
			else if (result == 2) {
				if (currentPlayer == 2)
					strcpy(jsonData, "{\"message\":\"You Win!\"}");
				else
					strcpy(jsonData, "{\"message\":\"You Lose...\"}");
			}
			else if (result == 3) {
				strcpy(jsonData, "{\"message\":\"Ends in a draw\"}");
			}else{
				//发送棋盘json
				//json data
				snprintf(jsonData, BUF_SMALL,
					"[{\"00\":%d,\"01\":%d,\"02\":%d},"
					"{\"10\":%d,\"11\":%d,\"12\":%d},"
					"{\"20\":%d,\"21\":%d,\"22\":%d}]",
					gameList[currentGame].chessboard[0][0], gameList[currentGame].chessboard[0][1], gameList[currentGame].chessboard[0][2],
					gameList[currentGame].chessboard[1][0], gameList[currentGame].chessboard[1][1], gameList[currentGame].chessboard[1][2],
					gameList[currentGame].chessboard[2][0], gameList[currentGame].chessboard[2][1], gameList[currentGame].chessboard[2][2]);
			}
		}
		else {
			strcpy(jsonData, 
				"[{\"00\":0,\"01\":0,\"02\":0},"
				 "{\"10\":0,\"11\":0,\"12\":0},"
				 "{\"20\":0,\"21\":0,\"22\":0}]");

		}
		printf("Json Data : ---- %s\n", jsonData);
		send(sock, protocol, strlen(protocol), 0);
		send(sock, servName, strlen(servName), 0);
		send(sock, contentType, strlen(contentType), 0);
		send(sock, jsonData, strlen(jsonData), 0);
		closesocket(sock);
	}
}

char *ContentType(char *file) {
	char extension[BUF_SMALL];
	char fileName[BUF_SMALL];
	strcpy(fileName, file);
	if (strstr(file, ".") != NULL) {
		strtok(fileName, ".");
		strcpy(extension, strtok(NULL, "."));
	}
	else {
		strcpy(extension, "NULL");
	}
	char temp[BUF_SMALL];
	if (!strcmp(extension, "html") || !strcmp(extension, "htm")) {
		strcpy(temp, "text/html");
	}
	else if(!strcmp(extension, "png") || !strcmp(extension, "jpg") || !strcmp(extension, "jpeg") || !strcmp(extension, "gif") || !strcmp(extension, "bmp")){
		snprintf(temp, BUF_SMALL, "image/%s", extension);
	}
	else {
		strcpy(temp, "text/html");
	}
	return temp;
}

void SendErrorMSG(SOCKET sock, char *errorNum) {
	char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
	char servName[] = "Server:Linux Web Server\r\n";
	char cntLen[] = "Content-length:2048\r\n";
	char cntType[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><title>400 Bad Request</title></head>"
		"<body><font size=+5><br>Error! Check the Request File Name and Method!"
		"</font></body></html>";

	/*transfer header info*/
	send(sock, protocol, strlen(protocol), 0);
	send(sock, servName, strlen(servName), 0);
	send(sock, cntLen, strlen(cntLen), 0);
	send(sock, cntType, strlen(cntType), 0);
	send(sock, content, strlen(content), 0);
	closesocket(sock);
}

void ErrorHandling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

//产生长度为length的随机字符串  
char* genRandomString(int length)
{
	length++;
	int flag, i;
	char* string;
	srand((unsigned)time(NULL));
	if ((string = (char*)malloc(length)) == NULL)
	{
		ErrorHandling((char*)"Create Cookie Error!");
		return NULL;
	}

	for (i = 0; i < length - 1; i++)
	{
		flag = rand() % 3;
		switch (flag)
		{
		case 0:
			string[i] = 'A' + rand() % 26;
			break;
		case 1:
			string[i] = 'a' + rand() % 26;
			break;
		case 2:
			string[i] = '0' + rand() % 10;
			break;
		default:
			string[i] = 'x';
			break;
		}
	}
	string[length - 1] = '\0';
	return string;
}

void loginGame(char *playerCookie) {
	int i = 0;
	int newGame = -1;
	for (i = 0; i < MAX_GAME_NUM; i++) {
		if (gameList[i].playerNumber == 1) {//某一玩家正在等待
			strcpy(gameList[i].player2Cookie, playerCookie);
			gameList[i].playerNumber++;
			//开始游戏
			//初始化棋盘
			int m, n;
			for (m = 0; m < 3; m++) {
				for (n = 0; n < 3; n++) {
					gameList[i].chessboard[m][n] = 0;
				}
			}
			gameList[i].turn = 1;//player1先走
			return;
		}
		else if (gameList[i].playerNumber == 0) {
			newGame = i;
		}
	}
	if (newGame == -1) {
		//局数已满
	}
	else {
		//创建新游戏局,并等待连接
		gameList[newGame].playerNumber++;
		strcpy(gameList[newGame].player1Cookie, playerCookie);
	}
	
}

int isEqual(char * str1, char * str2, int length)
{
	int i = 0;
	for (i = 0; i < length; i++) {
		if (str1[i] != str2[i])
			return 0;
	}
	return 1;
}

int judge(int gameIndex)
{
	int i, j;
	for (i = 0; i < 3; i++) {
		if (gameList[gameIndex].chessboard[0][i] == gameList[gameIndex].chessboard[1][i]
			&& gameList[gameIndex].chessboard[1][i] == gameList[gameIndex].chessboard[2][i]
			&& gameList[gameIndex].chessboard[2][i] == 1) {
			return 1;
		}
		else if (gameList[gameIndex].chessboard[0][i] == gameList[gameIndex].chessboard[1][i]
			&& gameList[gameIndex].chessboard[1][i] == gameList[gameIndex].chessboard[2][i]
			&& gameList[gameIndex].chessboard[2][i] == 2) {
			return 2;
		}
	}
	for (i = 0; i < 3; i++) {
		if (gameList[gameIndex].chessboard[i][0] == gameList[gameIndex].chessboard[i][1]
			&& gameList[gameIndex].chessboard[i][1] == gameList[gameIndex].chessboard[i][2]
			&& gameList[gameIndex].chessboard[i][2] == 1) {
			return 1;
		}
		else if (gameList[gameIndex].chessboard[i][0] == gameList[gameIndex].chessboard[i][1]
			&& gameList[gameIndex].chessboard[i][1] == gameList[gameIndex].chessboard[i][2]
			&& gameList[gameIndex].chessboard[i][2] == 2) {
			return 2;
		}
	}
	if (gameList[gameIndex].chessboard[0][0] == gameList[gameIndex].chessboard[1][1]
		&& gameList[gameIndex].chessboard[1][1] == gameList[gameIndex].chessboard[2][2]
		&& gameList[gameIndex].chessboard[2][2] == 1) {
		return 1;
	}
	if (gameList[gameIndex].chessboard[0][0] == gameList[gameIndex].chessboard[1][1]
		&& gameList[gameIndex].chessboard[1][1] == gameList[gameIndex].chessboard[2][2]
		&& gameList[gameIndex].chessboard[2][2] == 2) {
		return 2;
	}
	if (gameList[gameIndex].chessboard[0][2] == gameList[gameIndex].chessboard[1][1]
		&& gameList[gameIndex].chessboard[1][1] == gameList[gameIndex].chessboard[2][0]
		&& gameList[gameIndex].chessboard[2][0] == 1) {
		return 1;
	}
	if (gameList[gameIndex].chessboard[0][2] == gameList[gameIndex].chessboard[1][1]
		&& gameList[gameIndex].chessboard[1][1] == gameList[gameIndex].chessboard[2][0]
		&& gameList[gameIndex].chessboard[2][0] == 2) {
		return 2;
	}
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (gameList[gameIndex].chessboard[i][j] == 0) {
				return 0;
			}
		}
	}

	return 3;
}
