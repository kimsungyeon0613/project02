#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <ctype.h>

#define PORT_NUM 5050 // 서버 포트 번호
#define BUF_SIZE 1024
#define MAX_USERS 1000
#define MAX_ACCIDENTS 1000
#define MAX_INFO_LENGTH 30
#define USER_FILE "C:\\Coding\\project\\project02\\users.txt" // 사용자 정보 파일 경로
#define ACCIDENT_FILE "C:\\Coding\\project\\project02\\accident.txt"// 사고 정보 입력 파일 경로
#define SAMSUNG_ACCIDENT_FILE "C:\\Coding\\project\\project02\\samsung_AC.txt"// 삼성화재 사고 정보 파일 경로
#define SAMSUNG_CPS_FILE "C:\\Coding\\project\\project02\\samsung_CPS.txt" // 삼성화재 보험안내 정보 파일 경로
#define HANWHA_ACCIDENT_FILE "C:\\Coding\\project\\project02\\hanwha_AC.txt"    // 한화손해보험 사고 정보 파일 경로
#define HANWHA_CPS_FILE "C:\\Coding\\project\\project02\\hanwha_CPS.txt" // 한화손해보험 보험안내 정보 파일 경로
#define KB_ACCIDENT_FILE "C:\\Coding\\project\\project02\\kb_AC.txt" // KB손해보험 사고 정보 파일 경로
#define KB_CPS_FILE "C:\\Coding\\project\\project02\\kb_CPS.txt" // KB손해보험 보험안내 정보 파일 경로
#define DB_ACCIDENT_FILE "C:\\Coding\\project\\project02\\db_AC.txt" // DB손해보험 사고 정보 파일 경로
#define DB_CPS_FILE "C:\\Coding\\project\\project02\\db_CPS.txt" // DB손해보험 보험안내 정보 파일 경로
#define ACCIDENT_LAW_FILE "C:\\Coding\\project\\program02\\accident_law.txt" // 사고 법률 안내 파일 경로
#define ACCIDENT_GUIDE_FILE "C:\\Coding\\project\\program02\\accident_guide.txt" 

int stristr(const char* haystack, const char* needle) {
    while (*haystack) {
        const char *h = haystack, *n = needle;
        while (*h && *n && tolower(*h) == tolower(*n)) {
            h++; n++;
        }
        if (!*n) return 1;  // 매칭 성공
        haystack++;
    }
    return 0;  // 매칭 실패
}

void ErrorHandling(char* msg); 


int clientCount = 0;  // 클라이언트 수
SOCKET clientSocks[MAX_CLNT];  // 클라이언트 소켓 배열
HANDLE hMutex;  // 스레드 동기화 뮤텍스

typedef struct {
    int id[20];
    char password[20];
    char car_number[20];
    char car_type[20];
    char insurance_company[20];
}USER_DATA;

typedef struct {
    char id[20];
    char when[20];
    char what[20];
    char how[200];
    char role[20];  // 가해자/피해자
} ACCIDENT_DATA;

// 전역 변수
USER_DATA users[MAX_USERS];
ACCIDENT_DATA accidents[MAX_ACCIDENTS];
int user_count = 0;
int accident_count = 0;



// 사용자 인증 함수 (ID/PW만 확인)
int UserOk(const char *id, const char *pass) {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) {
        printf("사용자 정보 파일을 열 수 없습니다: %s\n", strerror(errno));
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';  // 개행 문자 제거
        
        // ID/PW/차번호/차종류/보험사 형식에서 ID와 PW만 추출
        char file_id[20] = {0};
        char file_pw[20] = {0};
        
        // sscanf로 ID와 PW만 파싱 (나머지 필드는 무시)
        if (sscanf(line, "%19[^/]/%19[^/]", file_id, file_pw) >= 2) {
            if (strcmp(id, file_id) == 0 && strcmp(pass, file_pw) == 0) {
                fclose(file);
                return 1;  // 인증 성공
            }
        }
    }
    
    fclose(file);
    return 0;  // 인증 실패
}

// 회원가입 함수
int save_user_data(const char* id, const char* password, const char* car_number, 
                  const char* car_type, const char* insurance_company) {
    // ID 유효성 검사 (공백이나 '/'가 포함되지 않아야 함)
    if (strpbrk(id, " /\t\n\r") != NULL) {
        printf("ID에 공백이나 '/'를 포함할 수 없습니다.\n");
        return 0;
    }
    
    // 비밀번호 유효성 검사
    if (strpbrk(password, " /\t\n\r") != NULL) {
        printf("비밀번호에 공백이나 '/'를 포함할 수 없습니다.\n");
        return 0;
    }

    // 먼저 ID 중복 확인
    FILE* check_file = fopen(USER_FILE, "r");
    if (check_file) {
        char line[256];
        while (fgets(line, sizeof(line), check_file)) {
            char existing_id[20] = {0};
            // 첫 번째 필드(ID)만 읽어옴
            if (sscanf(line, "%19[^/]", existing_id) == 1) {
                if (strcmp(id, existing_id) == 0) {
                    fclose(check_file);
                    printf("이미 존재하는 ID입니다. 다른 ID를 사용해주세요.\n");
                    return 0;  // 중복된 ID가 있음
                }
            }
        }
        fclose(check_file);
    }

    // 파일에 사용자 정보 추가 (추가 모드)
    FILE* file = fopen(USER_FILE, "a");
    if (file == NULL) {
        printf("사용자 정보 파일을 열 수 없습니다.\n");
        return 0;
    }

    // 새 사용자 정보를 파일에 추가 (한 줄에 모든 정보를 '/'로 구분)
    fprintf(file, "%s/%s/%s/%s/%s\n",
            id, password, car_number, car_type, insurance_company);
    
    fclose(file);
    printf("회원가입이 완료되었습니다.\n");
    return 1;  // 성공
}

// 사고 정보를 파일에 저장하는 함수
int ACCIDENT_DATA(const char* id, const char* when, const char* what, 
                 const char* how, const char* role) {
    // 파일을 추가 모드로 열기 (파일이 없으면 생성)
    FILE *f = fopen(ACCIDENT_FILE, "a");
    if (!f) {
        perror("사고 정보 파일을 열 수 없습니다");
        return 0;
    }

    // 파일에 사고 정보 추가 (탭으로 구분하여 저장)
    fprintf(f, "%s\t%s\t%s\t%s\t%s\n", id, when, what, how, role);
    
    fclose(f);
    return 1;  // 성공
}

// AI 명령어 처리 함수 (클라이언트로부터 받은 메시지 파싱 및 저장)
int handle_ai_command(const char* message) {
    char msg_copy[1024];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';

    // 메시지 형식: AI/ID/언제/무엇을/어떻게/가해자or피해자
    char *token = strtok(msg_copy, "/");
    if (!token || strcmp(token, "AI") != 0) {
        printf("잘못된 AI 명령어 형식입니다.\n");
        return 0;
    }

    // 각 필드 추출
    char *id = strtok(NULL, "/");
    char *when = strtok(NULL, "/");
    char *what = strtok(NULL, "/");
    char *how = strtok(NULL, "/");
    char *role = strtok(NULL, "/");

    // 필수 필드 검증
    if (!id || !when || !what || !how || !role) {
        printf("필수 필드가 누락되었습니다.\n");
        return 0;
    }

    // 역할 검증 (가해자 또는 피해자)
    if (strcmp(role, "가해자") != 0 && strcmp(role, "피해자") != 0) {
        printf("역할은 '가해자' 또는 '피해자'여야 합니다.\n");
        return 0;
    }

    // 사고 정보 저장
    if (ACCIDENT_DATA(id, when, what, how, role)) {
        printf("사고 정보가 성공적으로 저장되었습니다.\n");
        return 1;
    } else {
        printf("사고 정보 저장에 실패했습니다.\n");
        return 0;
    }
}

// 사용자 정보 업데이트 함수 (특정 사용자만 업데이트)
int USER_DATAUPDATE(const char* message, char* user_id) {
    if (!message || !user_id) {
        return 0;
    }

    // 메시지 파싱 (UF/필드번호/새값)
    char msg_copy[1024];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';
    
    char *token = strtok(msg_copy, "/");
    if (!token || strcmp(token, "UF") != 0) {
        return 0;
    }

    // 필드 번호 파싱
    char *field_str = strtok(NULL, "/");
    if (!field_str) {
        return 0;
    }
    int field = atoi(field_str);

    // 새 값 가져오기
    char *new_value = strtok(NULL, "/");
    if (!new_value) {
        return 0;
    }

    // 파일을 읽기 모드로 열어서 모든 사용자 정보를 메모리로 로드
    FILE *file = fopen(USER_FILE, "r");
    if (!file) {
        return 0;
    }

    // 모든 사용자 정보를 저장할 배열
    char users[100][256];  // 최대 100명의 사용자, 각 줄은 최대 255자
    int user_count = 0;
    int target_index = -1;
    char line[256];

    // 파일에서 모든 사용자 정보 읽기
    while (fgets(line, sizeof(line), file) && user_count < 100) {
        line[strcspn(line, "\r\n")] = '\0';  // 개행 문자 제거
        
        // 현재 라인의 사용자 ID 추출
        char current_id[50] = {0};
        strncpy(current_id, line, strcspn(line, "/"));
        
        // 타겟 사용자 정보인지 확인
        if (strcmp(current_id, user_id) == 0) {
            target_index = user_count;
        }
        
        strcpy(users[user_count], line);
        user_count++;
    }
    fclose(file);

    // 타겟 사용자를 찾지 못한 경우
    if (target_index == -1) {
        return 0;
    }

    // 타겟 사용자 정보 파싱 및 업데이트
    char *fields[5] = {0};
    char *saveptr;
    char line_copy[256];
    strcpy(line_copy, users[target_index]);
    
    // 각 필드 분리
    fields[0] = strtok_r(line_copy, "/", &saveptr);  // ID
    for (int i = 1; i < 5; i++) {
        fields[i] = strtok_r(NULL, "/", &saveptr);
    }

    // 필드 업데이트
    switch (field) {
        case 1:  // 비밀번호 변경
            if (fields[1]) strncpy(fields[1], new_value, 19);
            break;
        case 2:  // 차번호 변경
            if (fields[2]) strncpy(fields[2], new_value, 19);
            break;
        case 3:  // 차종류 변경
            if (fields[3]) strncpy(fields[3], new_value, 19);
            break;
        case 4:  // 보험사 변경
            if (fields[4]) strncpy(fields[4], new_value, 19);
            break;
        case 5:  // 전체 정보 변경
            {
                char *full_fields[5] = {0};
                char full_copy[256];
                strncpy(full_copy, new_value, sizeof(full_copy) - 1);
                full_copy[sizeof(full_copy) - 1] = '\0';
                
                full_fields[0] = strtok_r(full_copy, "/", &saveptr);  // ID (변경 안함)
                for (int i = 1; i < 5; i++) {
                    full_fields[i] = strtok_r(NULL, "/", &saveptr);
                    if (full_fields[i] && fields[i]) {
                        strncpy(fields[i], full_fields[i], 19);
                        fields[i][19] = '\0';
                    }
                }
            }
            break;
        default:
            return 0;
    }

    // 업데이트된 사용자 정보로 문자열 재구성
    snprintf(users[target_index], sizeof(users[target_index]), 
             "%s/%s/%s/%s/%s",
             fields[0], 
             fields[1] ? fields[1] : "", 
             fields[2] ? fields[2] : "", 
             fields[3] ? fields[3] : "", 
             fields[4] ? fields[4] : "");

    // 파일에 다시 쓰기 (모든 사용자 정보 유지)
    file = fopen(USER_FILE, "w");
    if (!file) {
        return 0;
    }

    // 모든 사용자 정보를 파일에 쓰기 (변경된 사용자 정보 포함)
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s\n", users[i]);
    }
    fclose(file);

    return 1;
}

unsigned WINAPI HandleClient(void* arg) {

    SOCKET clientSock = *((SOCKET*)arg);
    int strLen = 0, i;
    char msg[BUF_SIZE];
    int logged_in = 0;
    char user_id[50] = {0};

    while ((strLen = recv(clientSock, msg, sizeof(msg)-1, 0)) != 0) {
        msg[strLen] = '\0';
        printf("수신된 메시지: '%s'\n", msg);  // 디버깅 로그
     
        // AI 명령어 처리 (로그인 없이도 처리 가능)
        if (strncmp(msg, "AI/", 3) == 0) {
            handle_ai_command(msg);
            continue;
        }

        if (strncmp(msg, "lo/", 3) == 0) {
            char id[20], pass[20];
            char* first_slash = strchr(msg + 3, '/');
            if (first_slash) {
                int id_len = first_slash - (msg + 3);
                strncpy(id, msg + 3, id_len);
                id[id_len] = '\0';
                strcpy(pass, first_slash + 1);

                if (UserOk(id, pass)) {
                    logged_in = 1;
                    strncpy(user_id, id, sizeof(user_id) - 1);
                    send(clientSock, "OK:로그인 성공\n", strlen("OK:로그인 성공\n"), 0);
                    printf("사용자 로그인: %s\n", id);
                    
                } else {
                    send(clientSock, "아이디 또는 비밀번호가 일치하지 않습니다.\n", 
                         strlen("아이디 또는 비밀번호가 일치하지 않습니다.\n"), 0);
                    printf("로그인 실패: %s\n", id);
                }
            }
        } else if (logged_in) {
                // 로그인된 사용자의 요청 처리
                if (strncmp(msg, "new/", 2) == 0) {
                    
                } else if (strncmp(msg, "UF", 2) == 0) {
                    add_book(clientSock, msg);
                } else if (strncmp(msg, "3/", 2) == 0) {
                    delete_book(clientSock, msg);
                } else if (strncmp(msg, "4/", 2) == 0) {
                    update_book(clientSock, msg);
                } else if (strncmp(msg, "5/", 2) == 0) {
                    sort_rating_book(clientSock, msg);
                } else if (strncmp(msg, "6/", 2) == 0) {
                    char new_id[20], new_pw[20], new_car_num[20], new_car_type[20], new_insurance[20];
                    char* first_slash = strchr(msg + 3, '/');
                    if (first_slash) {
                        int id_len = first_slash - (msg + 3);
                        strncpy(new_id, msg + 3, id_len);
                        new_id[id_len] = '\0';
                        strcpy(new_pw, first_slash + 1);
                        char* second_slash = strchr(new_pw, '/');
                        if (second_slash) {
                            int pw_len = second_slash - new_pw;
                            strncpy(new_pw, new_pw, pw_len);
                            new_pw[pw_len] = '\0';
                            strcpy(new_car_num, second_slash + 1);
                            char* third_slash = strchr(new_car_num, '/');
                            if (third_slash) {
                                int car_num_len = third_slash - new_car_num;
                                strncpy(new_car_num, new_car_num, car_num_len);
                                new_car_num[car_num_len] = '\0';
                                strcpy(new_car_type, third_slash + 1);
                                char* fourth_slash = strchr(new_car_type, '/');
                                if (fourth_slash) {
                                    int car_type_len = fourth_slash - new_car_type;
                                    strncpy(new_car_type, new_car_type, car_type_len);
                                    new_car_type[car_type_len] = '\0';
                                    strcpy(new_insurance, fourth_slash + 1);
                                    if (save_user_data(new_id, new_pw, new_car_num, new_car_type, new_insurance)) {
                                        send(clientSock, "회원가입이 완료되었습니다.\n", 
                                            strlen("회원가입이 완료되었습니다.\n"), 0);
                                    } else {
                                        send(clientSock, "회원가입에 실패했습니다. 이미 존재하는 ID일 수 있습니다.\n",
                                            strlen("회원가입에 실패했습니다. 이미 존재하는 ID일 수 있습니다.\n"), 0);
                                    }
                                }
                            }
                        }
                    }
                } else if (strncmp(msg, "7/", 2) == 0) {
                    delete_user(clientSock, msg);
                } else if (strncmp(msg, "8/", 2) == 0) {
                    send(clientSock, "8", 1, 0);
                    break;
                } else if (strncmp(msg, "UF/", 3) == 0) {
                    USER_DATAUPDATE(msg, user_id);
                }
        }
    }

    printf("클라이언트 연결 종료: %s\n", user_id[0] ? user_id : "미인증 사용자");

    WaitForSingleObject(hMutex, INFINITE);
    for (i = 0; i < clientCount; i++) {
        if (clientSock == clientSocks[i]) {
            for (int j = i; j < clientCount - 1; j++) {
                clientSocks[j] = clientSocks[j + 1];
            }
            break;
        }
    }
    clientCount--;
    ReleaseMutex(hMutex);
    closesocket(clientSock);
    return 0;
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    WSADATA wsaData;
    SOCKET serverSock, clientSock;
    SOCKADDR_IN serverAddr, clientAddr;
    int clientAddrSize;
    HANDLE hThread;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error!");

    hMutex = CreateMutex(NULL, FALSE, NULL);
    serverSock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT_NUM);

    if (bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        ErrorHandling("bind() error");
    if (listen(serverSock, 5) == SOCKET_ERROR)
        ErrorHandling("listen() error");

    printf("서버가 시작되었습니다. 포트: %d\n", PORT_NUM);

    while (1) {
        clientAddrSize = sizeof(clientAddr);
        clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &clientAddrSize);
        WaitForSingleObject(hMutex, INFINITE);
        clientSocks[clientCount++] = clientSock;
        ReleaseMutex(hMutex);
        hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)&clientSock, 0, NULL);
        printf("연결된 클라이언트 IP: %s\n", inet_ntoa(clientAddr.sin_addr));
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}







void END_PROGRAM(SOCKET sock) {
    send(sock, "END_PROGRAM:OK\n", strlen("END_PROGRAM:OK\n"), 0);
    exit(0);
}


void ErrorHandling(char* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
