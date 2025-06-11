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
#define DB_AC_FILE "C:\\Coding\\project\\program02\\db_AC"
#define DB_CPS_FILE "C:\\Coding\\project\\program02\\db_CPS"
#define KB_AC_FILE "C:\\Coding\\project\\program02\\kb_AC"
#define KB_CPS_FILE "C:\\Coding\\project\\program02\\kb_CPS"

// 프로토콜 정의
#define LOGIN_CMD "lo"
#define REGISTER_CMD "new"

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
    FILE* fp = fopen("USER_FILE", "r");
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

    remove("USER_FILE");
    rename("USER_FILE_TEMP", "USER_FILE");
    return 1;
}

// 클라이언트 요청 처리 함수
void handle_client_request(SOCKET client_socket) {
    char buffer[BUF_SIZE];
    int str_len;
    
    while ((str_len = recv(client_socket, buffer, BUF_SIZE - 1, 0)) != 0) {
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
        else if (strncmp(buffer, "ac/", 3) == 0) {
            char id[20];
            if (sscanf(buffer + 3, "%s", id) == 1) {
                printf("사고 안내 요청: ID=%s\n", id);
                handle_accident_guide(client_socket, id);
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
    FILE* fp = fopen("USER_FILE", "r");
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
    FILE* fp = fopen("USER_FILE", "r");
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
    fp = fopen("USER_FILE", "a");
    if (fp == NULL) {
        printf("사용자 파일을 열 수 없습니다.\n");
        return 0;
    }

    fprintf(fp, "%s/%s/%s/%s/%s\n", id, pw, car_number, car_type, ins_company);
    fclose(fp);
    return 1;
}



int main(void) {
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
    return 0;
}










