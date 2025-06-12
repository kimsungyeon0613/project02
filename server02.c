// 프로젝트: 교통사고 처리 안내 시스템
// 작성자: 학번 : 2022243090  이름 : 김성연

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT            5548
#define BUF_SIZE        1000
#define USER_FILE "C:\\Coding\\project\\program02\\users.txt"
#define ACCIEDENT_FILE "C:\\Coding\\project\\program02\\acciedent.txt"
#define SAMSUNG_AC_FILE "C:\\Coding\\project\\program02\\samsung_AC.txt"
#define SAMSUNG_CPS_FILE "C:\\Coding\\project\\program02\\samsung_CPS.txt"
#define HANWHA_AC_FILE "C:\\Coding\\project\\program02\\hanwha_AC.txt"
#define HANWHA_CPS_FILE "C:\\Coding\\project\\program02\\hanwha_CPS.txt"
#define DB_AC_FILE "C:\\Coding\\project\\program02\\db_AC.txt"
#define DB_CPS_FILE "C:\\Coding\\project\\program02\\db_CPS.txt"
#define KB_AC_FILE "C:\\Coding\\project\\program02\\kb_AC.txt"
#define KB_CPS_FILE "C:\\Coding\\project\\program02\\kb_CPS.txt"
#define USER_TEMP_FILE "C:\\Coding\\project\\program02\\USER_TEMP.txt"
#define ACCIDENT_LAW_FILE "C:\\Coding\\project\\program02\\accident_law.txt"

// 프로토콜 정의
#define LOGIN_CMD "lo"
#define REGISTER_CMD "new"
#define UPDATE_CMD "UF"
#define ACCIDENT_GUIDE_CMD "AC"


// 사용자 정보
typedef struct User {
    char id[20];
    char pw[20];
    char car_number[20];
    char car_type[20];
    char ins_company[20];
    struct User* next;
} User;


//사고 정보 입력 구조체
typedef struct Accident {   
    char userID[20];           // 사용자 ID 
    char when[20];              // 언제
    char what[20];             // 사고 날짜
    char how[200];          //어떻게 사고가 발생했는지 ) 내가 상대 뒷차를 박았다 등
    char fault[20];         // 사고 내용
    struct Accident *next;     // 큐의 다음 노드
} Accident;



static User* user_head = NULL;
static Accident* AC_head = NULL;

// 크리티컬 섹션 선언
CRITICAL_SECTION cs_users;

// 보험사별 안내 정보 구조체
typedef struct {
    char company[20];          // 보험사명
    char accident_type[20];    // 사고 유형
    char description[1000];    // 안내 내용
} InsuranceGuide;

// 보험사별 안내 정보를 로드하는 함수
InsuranceGuide* load_insurance_guide(const char* company) {
    char filename[256];
    if (strcmp(company, "삼성화재") == 0) {
        strcpy(filename, SAMSUNG_AC_FILE);
    } else if (strcmp(company, "DB손해보험") == 0) {
        strcpy(filename, DB_AC_FILE);
    } else if (strcmp(company, "한화손해보험") == 0) {
        strcpy(filename, HANWHA_AC_FILE);
    } else if (strcmp(company, "KB손해보험") == 0) {
        strcpy(filename, KB_AC_FILE);
    } else {
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    if (!file) return NULL;

    InsuranceGuide* guide = (InsuranceGuide*)malloc(sizeof(InsuranceGuide));
    memset(guide, 0, sizeof(InsuranceGuide)); // 구조체 전체 초기화
    char line[1000];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "[사고유형]") != NULL) {
            sscanf(line, "[사고유형]%[^\n]", guide->accident_type);
            found = 1;
        } else if (found && strstr(line, "[/사고유형]") == NULL) {
            strcat(guide->description, line);
        } else if (found && strstr(line, "[/사고유형]") != NULL) {
            break;
        }
    }

    fclose(file);
    return guide;
}

// 사용자 정보 수정 처리 함수
int handle_user_update(const char* update_type, const char* user_id, const char* new_value) {
    FILE* fp = fopen(USER_FILE, "r");
    if (fp == NULL) {
        printf("사용자 파일을 열 수 없습니다.\n");
        return 0;
    }

    FILE* temp_fp = fopen("USER_FILE_TEMP", "w");
    if (temp_fp == NULL) {
        fclose(fp);
        printf("임시 파일을 생성할 수 없습니다.\n");
        return 0;
    }

    char line[256];
    char file_id[20], file_pw[20], car_number[20], car_type[20], ins_company[20];
    int found = 0;
    char new_pw[20], new_car_number[20], new_car_type[20], new_ins_company[20];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%[^/]/%[^/]/%[^/]/%[^/]/%[^\n]", 
                  file_id, file_pw, car_number, car_type, ins_company) == 5) {
            if (strcmp(user_id, file_id) == 0) {
                found = 1;
                // 수정 유형에 따라 처리
                switch (update_type[0]) {
                    case '1':  // 비밀번호 변경
                        fprintf(temp_fp, "%s/%s/%s/%s/%s\n", 
                               file_id, new_value, car_number, car_type, ins_company);
                        break;
                    case '2':  // 차번호 변경
                        fprintf(temp_fp, "%s/%s/%s/%s/%s\n", 
                               file_id, file_pw, new_value, car_type, ins_company);
                        break;
                    case '3':  // 차종류 변경
                        fprintf(temp_fp, "%s/%s/%s/%s/%s\n", 
                               file_id, file_pw, car_number, new_value, ins_company);
                        break;
                    case '4':  // 보험사 변경
                        fprintf(temp_fp, "%s/%s/%s/%s/%s\n", 
                               file_id, file_pw, car_number, car_type, new_value);
                        break;
                    case '5':  // 전체 정보 변경
                        if (sscanf(new_value, "%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]", 
                                 new_pw, new_car_number, new_car_type, new_ins_company) == 4) {
                            fprintf(temp_fp, "%s/%s/%s/%s/%s\n", 
                                   file_id, new_pw, new_car_number, new_car_type, new_ins_company);
                        } else {
                            fclose(fp);
                            fclose(temp_fp);
                            remove("USER_FILE_TEMP");
                            return 0;
                        }
                        break;
                    default:
                        fclose(fp);
                        fclose(temp_fp);
                        remove("USER_FILE_TEMP");
                        return 0;
                }
            } else {
                fprintf(temp_fp, "%s", line);
            }
        }
    }

    fclose(fp);
    fclose(temp_fp);

    if (!found) {
        remove("USER_FILE_TEMP");
        return 0;
    }

    remove(USER_FILE);
    rename("USER_FILE_TEMP", USER_FILE);
    return 1;
}

// 사고 정보 큐 구조체
typedef struct AccidentNode {
    char userID[20];
    char when[20];
    char what[20];
    char how[200];
    char fault[20];
    struct AccidentNode* next;
} AccidentNode;

static AccidentNode* accident_head = NULL;
static AccidentNode* accident_tail = NULL;
CRITICAL_SECTION cs_accident;

// 사고 정보를 큐에 삽입
void enqueue_accident(const char* userID, const char* when, const char* what, const char* how, const char* fault) {
    AccidentNode* node = (AccidentNode*)malloc(sizeof(AccidentNode));
    strcpy(node->userID, userID);
    strcpy(node->when, when);
    strcpy(node->what, what);
    strcpy(node->how, how);
    strcpy(node->fault, fault);
    node->next = NULL;
    EnterCriticalSection(&cs_accident);
    if (accident_tail) {
        accident_tail->next = node;
        accident_tail = node;
    } else {
        accident_head = accident_tail = node;
    }
    LeaveCriticalSection(&cs_accident);
}

// 사고 정보를 큐에서 꺼내어 파일에 저장
void save_accident_to_file() {
    EnterCriticalSection(&cs_accident);
    FILE* fp = fopen(ACCIEDENT_FILE, "a");
    if (!fp) {
        LeaveCriticalSection(&cs_accident);
        return;
    }
    while (accident_head) {
        AccidentNode* node = accident_head;
        fprintf(fp, "%s/%s/%s/%s/%s\n", node->userID, node->when, node->what, node->how, node->fault);
        accident_head = node->next;
        free(node);
    }
    accident_tail = NULL;
    fclose(fp);
    LeaveCriticalSection(&cs_accident);
}

// 클라이언트 요청 처리 함수
void handle_client_request(SOCKET client_socket) {
    char buffer[BUF_SIZE];
    int str_len;
    
    while ((str_len = recv(client_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[str_len] = '\0';
        printf("수신된 데이터: %s\n", buffer);

        // 로그인 요청 처리
        if (strncmp(buffer, "lo/", 3) == 0) {
            char id[20], pw[20];
            if (sscanf(buffer + 3, "%[^/]/%s", id, pw) == 2) {
                printf("로그인 시도: ID=%s\n", id);
                int result = handle_login(id, pw);
                char response[2] = {result + '0', '\0'};
                send(client_socket, response, 1, 0);
            }
        }
        // 회원가입 요청 처리
        else if (strncmp(buffer, "new/", 4) == 0) {
            char id[20], pw[20], car_number[20], car_type[20], ins_company[20];
            if (sscanf(buffer + 4, "%[^/]/%[^/]/%[^/]/%[^/]/%s", 
                      id, pw, car_number, car_type, ins_company) == 5) {
                printf("회원가입 시도: ID=%s\n", id);
                int result = handle_register(id, pw, car_number, car_type, ins_company);
                char response[2] = {result + '0', '\0'};
                send(client_socket, response, 1, 0);
            }
        }
        // 사용자 정보 수정 요청 처리
        else if (strncmp(buffer, "UF/", 3) == 0) {
            char update_type[2], id[20], new_value[256];
            if (sscanf(buffer + 3, "%[^/]/%[^/]/%[^\n]", update_type, id, new_value) == 3) {
                printf("정보 수정 시도: ID=%s, 유형=%s\n", id, update_type);
                int result = handle_user_update(update_type, id, new_value);
                char response[2] = {result + '0', '\0'};
                send(client_socket, response, 1, 0);
            }
        }
        // 사고 안내 요청 처리
        else if (strncmp(buffer, "AC/", 3) == 0) {
            char id[20];
            if (sscanf(buffer + 3, "%s", id) == 1) {
                printf("사고 안내 요청: ID=%s\n", id);
                handle_accident_guide(client_socket, id);
            }
        }
        // 12대 중과실 법률 안내 요청 처리
        else if (strcmp(buffer, "LAW") == 0) {
            law_data(client_socket);
        }
        // 사고 정보 저장 (ID/언제/무엇을/어떻게/가해자or피해자)
        else {
            char userID[20], when[20], what[20], how[200], fault[20];
            if (sscanf(buffer, "%19[^/]/%19[^/]/%19[^/]/%199[^/]/%19[^\n]", userID, when, what, how, fault) == 5) {
                enqueue_accident(userID, when, what, how, fault);
                save_accident_to_file();
                send(client_socket, "1", 1, 0);
            } else {
                send(client_socket, "0", 1, 0);
            }
        }
    }
}

// 에러 처리 함수
void ErrorHandling(const char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

// 로그인 처리 함수
int handle_login(const char* id, const char* pw) {
    FILE* fp = fopen(USER_FILE, "r");
    if (fp == NULL) {
        printf("사용자 파일을 열 수 없습니다.\n");
        return 0;
    }

    char line[256];
    char file_id[20], file_pw[20], car_number[20], car_type[20], ins_company[20];
    
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%[^/]/%[^/]/%[^/]/%[^/]/%[^\n]", 
                  file_id, file_pw, car_number, car_type, ins_company) == 5) {
            if (strcmp(id, file_id) == 0 && strcmp(pw, file_pw) == 0) {
                fclose(fp);
                // 로그인 성공 시 User 구조체에 정보 저장
                if (user_head != NULL) {
                    free(user_head);
                }
                user_head = (User*)malloc(sizeof(User));
                strcpy(user_head->id, file_id);
                strcpy(user_head->pw, file_pw);
                strcpy(user_head->car_number, car_number);
                strcpy(user_head->car_type, car_type);
                strcpy(user_head->ins_company, ins_company);
                user_head->next = NULL;
                return 1;
            }
        }
    }
    
    fclose(fp);
    return 0;
}

// 회원가입 처리 함수
int handle_register(const char* id, const char* pw, const char* car_number, 
                   const char* car_type, const char* ins_company) {
    // ID 중복 확인
    FILE* fp = fopen(USER_FILE, "r");
    if (fp != NULL) {
        char line[256];
        char file_id[20];
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "%[^/]", file_id) == 1) {
                if (strcmp(id, file_id) == 0) {
                    fclose(fp);
                    return 0;  // ID 중복
                }
            }
        }
        fclose(fp);
    }

    // 새 사용자 정보 저장
    fp = fopen(USER_FILE, "a");
    if (fp == NULL) {
        printf("사용자 파일을 열 수 없습니다.\n");
        return 0;
    }

    fprintf(fp, "%s/%s/%s/%s/%s\n", id, pw, car_number, car_type, ins_company);
    fclose(fp);
    return 1;
}

// 사고 입력 정보
void accident_data(SOCKET client_socket, const char* id) {
    char buffer[BUF_SIZE];
    Accident* new_accident = (Accident*)malloc(sizeof(Accident));
    if (new_accident == NULL) {
        ErrorHandling("메모리 할당 실패");
    }

    // 사용자 ID 설정
    strcpy(new_accident->userID, id);

    // 사고 정보 입력
    printf("사고 발생 일시: ");
    fgets(new_accident->when, sizeof(new_accident->when), stdin);
    new_accident->when[strcspn(new_accident->when, "\r\n")] = '\0';

    printf("사고 장소: ");
    fgets(new_accident->what, sizeof(new_accident->what), stdin);
    new_accident->what[strcspn(new_accident->what, "\r\n")] = '\0';

    printf("사고 내용: ");
    fgets(new_accident->how, sizeof(new_accident->how), stdin);
    new_accident->how[strcspn(new_accident->how, "\r\n")] = '\0';

    printf("가해자/피해자 (선택): ");
    fgets(new_accident->fault, sizeof(new_accident->fault), stdin);
    new_accident->fault[strcspn(new_accident->fault, "\r\n")] = '\0';

    // 큐에 삽입
    EnterCriticalSection(&cs_users);
    new_accident->next = AC_head;
    AC_head = new_accident;
    LeaveCriticalSection(&cs_users);

    // 서버에 사고 정보 전송
    sprintf(buffer, "%s/%s/%s/%s/%s", new_accident->userID, 
            new_accident->when, new_accident->what, 
            new_accident->how, new_accident->fault);
    
    send(client_socket, buffer, strlen(buffer), 0);
}

void law_data(SOCKET client_socket) {
    char buffer[BUF_SIZE];
    // 서버에 LAW 명령어 전송
    strcpy(buffer, "LAW");
    if (send(client_socket, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");

    printf("\n[12대 중과실 법률 안내]\n");
    // 여러 번 recv()로 모두 받기
    while (1) {
        int strLen = recv(client_socket, buffer, BUF_SIZE - 1, 0);
        if (strLen <= 0) break; // 연결 종료 또는 에러
        buffer[strLen] = '\0';
        printf("%s", buffer);
        if (strLen < BUF_SIZE - 1) break; // 마지막 데이터일 가능성
    }
    printf("\n");
}

int main(void) {
    SetConsoleOutputCP(CP_UTF8); // UTF-8 출력 설정
    WSADATA wsaData;
    SOCKET serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    int clnt_addr_size;

    // WinSock 초기화
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error");

    // 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == INVALID_SOCKET)
        ErrorHandling("socket() error");

    // 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    // 소켓에 주소 할당
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        ErrorHandling("bind() error");

    // 연결 요청 대기
    if (listen(serv_sock, 5) == SOCKET_ERROR)
        ErrorHandling("listen() error");

    printf("서버가 시작되었습니다. 포트: %d\n", PORT);

    InitializeCriticalSection(&cs_accident);

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == INVALID_SOCKET)
            ErrorHandling("accept() error");

        printf("클라이언트가 연결되었습니다.\n");
        handle_client_request(clnt_sock);
        closesocket(clnt_sock);
    }

    closesocket(serv_sock);
    WSACleanup();
    DeleteCriticalSection(&cs_accident);
    return 0;
}










