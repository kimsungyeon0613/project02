#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP   "172.30.1.12"
#define SERVER_PORT 5548
#define BUF_SIZE    256

void ErrorHandling(const char* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

int main(void) {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in servAddr;
    char buffer[BUF_SIZE];
    int strLen;

    char id[50], pw[50], cmd[BUF_SIZE];

    // 콘솔 UTF-8 설정
    SetConsoleOutputCP(CP_UTF8);

    // WinSock 초기화
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error");

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        ErrorHandling("socket() error");

    // 서버 주소 설정
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servAddr.sin_port        = htons(SERVER_PORT);

    // 서버에 연결
    if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        ErrorHandling("connect() error");

    fputs("서버에 연결되었습니다.\n\n", stdout);

    // --- 로그인 루프 ---
    while (1) {
        fputs("아이디: ", stdout); fflush(stdout);
        if (!fgets(id, sizeof(id), stdin))
            ErrorHandling("입력 오류");
        id[strcspn(id, "\r\n")] = '\0';

        fputs("비밀번호: ", stdout); fflush(stdout);
        if (!fgets(pw, sizeof(pw), stdin))
            ErrorHandling("입력 오류");
        pw[strcspn(pw, "\r\n")] = '\0';

        // 로그인 명령 전송
        int len = snprintf(buffer, BUF_SIZE, "lo/%s/%s", id, pw);
        if (send(sock, buffer, len, 0) == SOCKET_ERROR)
            ErrorHandling("send() error");

        // 서버 응답 수신
        strLen = recv(sock, buffer, BUF_SIZE - 1, 0);
        if (strLen <= 0)
            ErrorHandling("서버로부터 응답이 없습니다.");
        buffer[strLen] = '\0';

        fputs("서버 응답: ", stdout);
        fputs(buffer, stdout);
        if (buffer[strlen(buffer)-1] != '\n')
            fputc('\n', stdout);

        if (strncmp(buffer, "OK:", 3) == 0)
            break;
    }

    // --- 초기 도서 목록 출력 ---
    fputs("\n=== 도서 목록 ===\n", stdout);
    while ((strLen = recv(sock, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (strcmp(buffer, "[END]") == 0)
            break;
        fputs(buffer, stdout);
        fputc('\n', stdout);
    }

    // --- 명령 루프 ---
    for (;;) {
        fputs("\n메뉴:\n", stdout);
        fputs("1. 도서 검색\n", stdout);
        fputs("2. 도서 추가\n", stdout);
        fputs("3. 도서 삭제\n", stdout);
        fputs("4. 도서 수정\n", stdout);
        fputs("5. 평점 정렬\n", stdout);
        fputs("6. 사용자 추가\n", stdout);
        fputs("7. 사용자 삭제\n", stdout);
        fputs("8. 종료\n", stdout);
        fputs("선택: ", stdout); fflush(stdout);

        if (!fgets(cmd, sizeof(cmd), stdin))
            break;
        cmd[strcspn(cmd, "\r\n")] = '\0';

        // 옵션별 명령어 구성
        if (strcmp(cmd, "1") == 0) {
            fputs("검색 키워드: ", stdout); fflush(stdout);
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\r\n")] = '\0';
            snprintf(cmd, BUF_SIZE, "1 %s", buffer);
        }
        else if (strcmp(cmd, "2") == 0) {
            char title[100], author[100], rating[20];
            fputs("제목: ", stdout); fflush(stdout);
            fgets(title, sizeof(title), stdin);
            title[strcspn(title, "\r\n")] = '\0';
            fputs("저자: ", stdout); fflush(stdout);
            fgets(author, sizeof(author), stdin);
            author[strcspn(author, "\r\n")] = '\0';
            fputs("평점: ", stdout); fflush(stdout);
            fgets(rating, sizeof(rating), stdin);
            rating[strcspn(rating, "\r\n")] = '\0';
            snprintf(cmd, BUF_SIZE, "2 %s %s %s", title, author, rating);
        }
        else if (strcmp(cmd, "3") == 0) {
            fputs("삭제 키워드: ", stdout); fflush(stdout);
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\r\n")] = '\0';
            snprintf(cmd, BUF_SIZE, "3 %s", buffer);
        }
        else if (strcmp(cmd, "4") == 0) {
            char key[100], newTitle[100], newAuthor[100], newRating[20];
            fputs("검색 키워드: ", stdout); fflush(stdout);
            fgets(key, sizeof(key), stdin);
            key[strcspn(key, "\r\n")] = '\0';
            fputs("새 제목: ", stdout); fflush(stdout);
            fgets(newTitle, sizeof(newTitle), stdin);
            newTitle[strcspn(newTitle, "\r\n")] = '\0';
            fputs("새 저자: ", stdout); fflush(stdout);
            fgets(newAuthor, sizeof(newAuthor), stdin);
            newAuthor[strcspn(newAuthor, "\r\n")] = '\0';
            fputs("새 평점: ", stdout); fflush(stdout);
            fgets(newRating, sizeof(newRating), stdin);
            newRating[strcspn(newRating, "\r\n")] = '\0';
            snprintf(cmd, BUF_SIZE, "4 %s %s %s %s",
                     key, newTitle, newAuthor, newRating);
        }
        else if (strcmp(cmd, "5") == 0) {
            strcpy(cmd, "5");
        }
        else if (strcmp(cmd, "6") == 0) {
            fputs("추가할 사용자 ID: ", stdout); fflush(stdout);
            fgets(id, sizeof(id), stdin);
            id[strcspn(id, "\r\n")] = '\0';
            fputs("추가할 사용자 PW: ", stdout); fflush(stdout);
            fgets(pw, sizeof(pw), stdin);
            pw[strcspn(pw, "\r\n")] = '\0';
            snprintf(cmd, BUF_SIZE, "6 %s %s", id, pw);
        }
        else if (strcmp(cmd, "7") == 0) {
            fputs("삭제할 사용자 ID: ", stdout); fflush(stdout);
            fgets(id, sizeof(id), stdin);
            id[strcspn(id, "\r\n")] = '\0';
            fputs("삭제할 사용자 PW: ", stdout); fflush(stdout);
            fgets(pw, sizeof(pw), stdin);
            pw[strcspn(pw, "\r\n")] = '\0';
            snprintf(cmd, BUF_SIZE, "7 %s %s", id, pw);
        }
        else if (strcmp(cmd, "8") == 0) {
            // 종료 명령 전송 후 루프 탈출
            if (send(sock, "8", 1, 0) == SOCKET_ERROR)
                ErrorHandling("send() error");
            break;
        }
        else {
            fputs("잘못된 선택입니다.\n", stdout);
            continue;
        }

        // (8번이 아닌 경우) 명령 전송
        if (strcmp(cmd, "8") != 0) {
            if (send(sock, cmd, (int)strlen(cmd), 0) == SOCKET_ERROR)
                ErrorHandling("send() error");
        }

        // 응답 수신 및 출력
        while ((strLen = recv(sock, buffer, BUF_SIZE - 1, 0)) > 0) {
            buffer[strcspn(buffer, "\r\n")] = '\0';
            if (strcmp(buffer, "[END]") == 0)
                break;
            fputs(buffer, stdout);
            fputc('\n', stdout);
        }
    }

    fputs("연결 종료\n", stdout);
    closesocket(sock);
    WSACleanup();
    return 0;
}