// server.c
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT            5548
#define BUF_SIZE        1000
#define USER_FILE       "C:\\Coding\\project\\program01\\users.txt"
#define BOOK_FILE       "C:\\Coding\\project\\program01\\booklist2.txt"

// 사용자 정보
typedef struct User {
    char id[50];
    char pw[50];
    struct User* next;
} User;

// 도서 정보
typedef struct Book {
    int num;
    char title[100];
    char author[100];
    float rating;
    struct Book* next;
} Book;

// 전역 리스트 헤드
static User* user_head = NULL;
static Book* book_head = NULL;

// 동기화
static CRITICAL_SECTION cs_users, cs_books;

// 함수 선언
void    load_users(void);
void    load_books(void);
void    save_users(void);
void    save_books(void);
int     check_login(const char* id, const char* pw);
int     add_user(const char* id, const char* pw);
int     remove_user(const char* id, const char* pw);

void    list_books(SOCKET sock);
void    search_books(SOCKET sock, const char* keyword);
void    add_book(const char* title, const char* author, float rating);
void    delete_book(const char* keyword);
void    update_book(const char* key, const char* new_title,
                    const char* new_author, float new_rating);
void    sort_books_by_rating(SOCKET sock);

unsigned __stdcall client_handler(void* arg);

// 에러 출력 후 프로세스 종료
void ErrorHandling(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    WSADATA wsa;
    SOCKET listenSock;
    struct sockaddr_in servAddr;

    SetConsoleOutputCP(CP_UTF8);
    InitializeCriticalSection(&cs_users);
    InitializeCriticalSection(&cs_books);

    load_users();
    load_books();

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        ErrorHandling("WSAStartup() error");

    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
        ErrorHandling("socket() error");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(PORT);

    if (bind(listenSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        ErrorHandling("bind() error");

    if (listen(listenSock, 5) == SOCKET_ERROR)
        ErrorHandling("listen() error");

    printf("서버 시작: 포트 %d, 접속 대기중...\n", PORT);

    while (1) {
        SOCKET clientSock = accept(listenSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) {
            perror("accept() error");
            continue;
        }
        // 새 클라이언트용 스레드 생성
        _beginthreadex(NULL, 0, client_handler, (void*)(intptr_t)clientSock, 0, NULL);
    }

    // (실제로는 도달하지 않음)
    DeleteCriticalSection(&cs_users);
    DeleteCriticalSection(&cs_books);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}

// --- 파일 I/O ---
void load_users(void) {
    FILE* fp = fopen(USER_FILE, "r");
    if (!fp) return;
    char line[150];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        char* sep = strstr(line, "//");
        if (!sep) continue;
        *sep = '\0';
        User* u = malloc(sizeof(User));
        if (!u) ErrorHandling("메모리 할당 실패");
        strcpy(u->id, line);
        strcpy(u->pw, sep + 2);
        EnterCriticalSection(&cs_users);
        u->next = user_head;
        user_head = u;
        LeaveCriticalSection(&cs_users);
    }
    fclose(fp);
}

void save_users(void) {
    char buf[150];
    EnterCriticalSection(&cs_users);
    FILE* fp = fopen(USER_FILE, "w");
    if (!fp) { LeaveCriticalSection(&cs_users); return; }
    for (User* u = user_head; u; u = u->next) {
        snprintf(buf, sizeof(buf), "%s//%s\n", u->id, u->pw);
        fputs(buf, fp);
    }
    fclose(fp);
    LeaveCriticalSection(&cs_users);
}

void load_books(void) {
    FILE* fp = fopen(BOOK_FILE, "r");
    if (!fp) return;
    char line[300];
    while (fgets(line, sizeof(line), fp)) {
        Book* b = malloc(sizeof(Book));
        if (!b) ErrorHandling("메모리 할당 실패");
        if (sscanf(line, "%d\t%99[^\t]\t%99[^\t]\t%f",
                   &b->num, b->title, b->author, &b->rating) != 4) {
            free(b);
            continue;
        }
        EnterCriticalSection(&cs_books);
        b->next = book_head;
        book_head = b;
        LeaveCriticalSection(&cs_books);
    }
    fclose(fp);
}

void save_books(void) {
    char buf[300];
    EnterCriticalSection(&cs_books);
    FILE* fp = fopen(BOOK_FILE, "w");
    if (!fp) { LeaveCriticalSection(&cs_books); return; }
    for (Book* b = book_head; b; b = b->next) {
        snprintf(buf, sizeof(buf), "%d\t%s\t%s\t%.1f\n",
                 b->num, b->title, b->author, b->rating);
        fputs(buf, fp);
    }
    fclose(fp);
    LeaveCriticalSection(&cs_books);
}

// --- 사용자 관리 ---
int check_login(const char* id, const char* pw) {
    EnterCriticalSection(&cs_users);
    for (User* u = user_head; u; u = u->next) {
        if (strcmp(u->id, id) == 0 && strcmp(u->pw, pw) == 0) {
            LeaveCriticalSection(&cs_users);
            return 1;
        }
    }
    LeaveCriticalSection(&cs_users);
    return 0;
}

int add_user(const char* id, const char* pw) {
    EnterCriticalSection(&cs_users);
    for (User* u = user_head; u; u = u->next) {
        if (strcmp(u->id, id) == 0) {
            LeaveCriticalSection(&cs_users);
            return 0;
        }
    }
    User* u = malloc(sizeof(User));
    if (!u) ErrorHandling("메모리 할당 실패");
    strcpy(u->id, id);
    strcpy(u->pw, pw);
    u->next = user_head;
    user_head = u;
    save_users();
    LeaveCriticalSection(&cs_users);
    return 1;
}

int remove_user(const char* id, const char* pw) {
    EnterCriticalSection(&cs_users);
    User* prev = NULL;
    for (User* u = user_head; u; prev = u, u = u->next) {
        if (strcmp(u->id, id) == 0 && strcmp(u->pw, pw) == 0) {
            if (prev) prev->next = u->next;
            else user_head = u->next;
            free(u);
            save_users();
            LeaveCriticalSection(&cs_users);
            return 1;
        }
    }
    LeaveCriticalSection(&cs_users);
    return 0;
}

// --- 도서 관리 ---
void list_books(SOCKET sock) {
    char buf[BUF_SIZE];
    EnterCriticalSection(&cs_books);
    for (Book* b = book_head; b; b = b->next) {
        int n = snprintf(buf, BUF_SIZE, "%d\t%s\t%s\t%.1f\n",
                         b->num, b->title, b->author, b->rating);
        send(sock, buf, n, 0);
    }
    LeaveCriticalSection(&cs_books);
    send(sock, "[END]\n", 6, 0);
}

void search_books(SOCKET sock, const char* keyword) {
    char buf[BUF_SIZE];
    EnterCriticalSection(&cs_books);
    for (Book* b = book_head; b; b = b->next) {
        if (strstr(b->title, keyword) || strstr(b->author, keyword)) {
            int n = snprintf(buf, BUF_SIZE, "%d\t%s\t%s\t%.1f\n",
                             b->num, b->title, b->author, b->rating);
            send(sock, buf, n, 0);
        }
    }
    LeaveCriticalSection(&cs_books);
    send(sock, "[END]\n", 6, 0);
}

void add_book(const char* title, const char* author, float rating) {
    EnterCriticalSection(&cs_books);
    int max = 0;
    for (Book* b = book_head; b; b = b->next)
        if (b->num > max) max = b->num;
    Book* nb = malloc(sizeof(Book));
    if (!nb) ErrorHandling("메모리 할당 실패");
    nb->num = max + 1;
    strncpy(nb->title, title, 99); nb->title[99] = '\0';
    strncpy(nb->author, author, 99); nb->author[99] = '\0';
    nb->rating = rating;
    nb->next = book_head;
    book_head = nb;
    save_books();
    LeaveCriticalSection(&cs_books);
}

void delete_book(const char* keyword) {
    EnterCriticalSection(&cs_books);
    Book* prev = NULL;
    for (Book* b = book_head; b; prev = b, b = b->next) {
        if (strstr(b->title, keyword) || strstr(b->author, keyword)) {
            if (prev) prev->next = b->next;
            else book_head = b->next;
            free(b);
            break;
        }
    }
    save_books();
    LeaveCriticalSection(&cs_books);
}

void update_book(const char* key, const char* new_title,
                 const char* new_author, float new_rating) {
    EnterCriticalSection(&cs_books);
    for (Book* b = book_head; b; b = b->next) {
        if (strstr(b->title, key) || strstr(b->author, key)) {
            strncpy(b->title, new_title, sizeof(b->title)-1);
            b->title[sizeof(b->title)-1] = '\0';
            strncpy(b->author, new_author, sizeof(b->author)-1);
            b->author[sizeof(b->author)-1] = '\0';
            b->rating = new_rating;
            break;
        }
    }
    save_books();
    LeaveCriticalSection(&cs_books);
}

// 간단히 현재 그대로 목록만 재전송
void sort_books_by_rating(SOCKET sock) {
    list_books(sock);
}

// --- 클라이언트 처리 스레드 ---
unsigned __stdcall client_handler(void* arg) {
    SOCKET sock = (SOCKET)(intptr_t)arg;
    char buf[BUF_SIZE];
    int len;

    // 1) 로그인
    if ((len = recv(sock, buf, BUF_SIZE-1, 0)) <= 0) goto CLEANUP;
    buf[len] = '\0';
    {
        char id[50], pw[50];
        if (sscanf(buf, "lo/%49[^/]/%49s", id, pw) != 2 ||
            !check_login(id, pw)) {
            send(sock, "ERROR:로그인 실패\n", 18, 0);
            goto CLEANUP;
        }
        send(sock, "OK:로그인 성공\n", 14, 0);
    }
    list_books(sock);

    // 2) 명령 루프
    while ((len = recv(sock, buf, BUF_SIZE-1, 0)) > 0) {
        buf[len] = '\0';
        switch (buf[0]) {
        case '1': // 검색
            search_books(sock, buf + 2);
            break;
        case '2': { // 추가
            char t[100], a[100], rstr[20];
            if (sscanf(buf+2, "%99s %99s %19s", t, a, rstr)==3) {
                add_book(t, a, strtof(rstr, NULL));
                send(sock, "OK:도서 추가 성공\n", 17, 0);
                send(sock, "[END]\n", 6, 0);
            }
            break;
        }
        case '3': // 삭제
            delete_book(buf + 2);
            send(sock, "OK:도서 삭제 성공\n", 17, 0);
            send(sock, "[END]\n", 6, 0);
            break;
        case '4': { // 수정
            char key[100], nt[100], na[100], nr[20];
            if (sscanf(buf+2, "%99s %99s %99s %19s",
                       key, nt, na, nr) == 4) {
                update_book(key, nt, na, strtof(nr, NULL));
                send(sock, "OK:도서 수정 성공\n", 17, 0);
                send(sock, "[END]\n", 6, 0);
            }
            break;
        }
        case '5': // 정렬
            sort_books_by_rating(sock);
            break;
        case '6': { // 사용자 추가
            char id[50], pw[50];
            if (sscanf(buf+2, "%49s %49s", id, pw)==2) {
                if (add_user(id,pw))
                    send(sock, "OK:사용자 추가 성공\n", 19, 0);
                else
                    send(sock, "ERROR:사용자 중복\n", 16, 0);
            }
            send(sock, "[END]\n", 6, 0);
            break;
        }
        case '7': { // 사용자 삭제
            char id[50], pw[50];
            if (sscanf(buf+2, "%49s %49s", id, pw)==2) {
                if (remove_user(id,pw))
                    send(sock, "OK:사용자 삭제 성공\n", 19, 0);
                else
                    send(sock, "ERROR:사용자 없음\n", 16, 0);
            }
            send(sock, "[END]\n", 6, 0);
            break;
        }
        case '8': // 종료
            send(sock, "OK:종료\n", 7, 0);
            goto CLEANUP;
        default:
            send(sock, "ERROR:알 수 없는 명령\n", 21, 0);
            send(sock, "[END]\n", 6, 0);
            break;
        }
    }

CLEANUP:
    closesocket(sock);
    return 0;
}
