// 프로젝트: 교통사고 처리 안내 시스템
// 작성자: 학번 : 2022243090  이름 : 김성연

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 1024
#define MAX_USERS 100
#define MAX_ACCIDENTS 100
#define MAX_INFO_LENGTH 30
#define USER_FILE "C:\\Coding\\project\\project02\\users.txt" // 사용자 정보 파일 경로
#define USER_TEMP_FILE "C:\\Coding\\project\\project02\\users_temp.txt"// 사용자 정보 임시 파일 경로
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


// 사용자 정보 구조체
typedef struct {
    char id[MAX_INFO_LENGTH];
    char password[MAX_INFO_LENGTH];
    char car_number[MAX_INFO_LENGTH];
    char car_type[MAX_INFO_LENGTH];
    char insurance_company[MAX_INFO_LENGTH];
} USER_DATA;

// 보험사별 안내 정보 구조체
typedef struct {
    char id[MAX_INFO_LENGTH];
    char accident_date[MAX_INFO_LENGTH];
    char accident_type[MAX_INFO_LENGTH];
    char accident_description[MAX_INFO_LENGTH];
    char accident_role[MAX_INFO_LENGTH];  // 가해자/피해자
} ACCIDENT_DATA;

// 전역 변수
USER_DATA users[MAX_USERS];
ACCIDENT_DATA accidents[MAX_ACCIDENTS];
int user_count = 0;
int accident_count = 0;

// 함수 선언
void handle_client(SOCKET client_socket);
void process_login(char* buffer, SOCKET client_socket);
void process_register(char* buffer, SOCKET client_socket);
void process_user_update(char* buffer, SOCKET client_socket);
void process_insurance_info(char* buffer, SOCKET client_socket);
void process_accident_info(char* buffer, SOCKET client_socket);
void load_user_data();
void save_user_data();
void load_accident_data();
void save_accident_data();
void accident_data(SOCKET sock, const char* id);
void process_insurance_compensation_info(char* buffer, SOCKET client_socket);
int handle_login(const char* id, const char* pw);
int handle_register(const char* id, const char* pw, const char* car_number, const char* car_type, const char* ins_company);
void LAW_guide(SOCKET client_socket);
void LAW_data(SOCKET client_socket);

int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_size;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup 실패\n");
        return 1;
    }

    // 사용자 및 사고 데이터 로드
    load_user_data();
    load_accident_data();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("소켓 생성 실패\n");
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(5548);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("바인드 실패\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("리스닝 실패\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("서버가 시작되었습니다. 포트: 5548\n");

    while (1) {
        client_addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_socket == INVALID_SOCKET) {
            printf("클라이언트 연결 실패\n");
            continue;
        }
        // 멀티스레드로 클라이언트 처리
        _beginthread((void(*)(void*))handle_client, 0, (void*)client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    getchar();
    return 0;
}

void handle_client(SOCKET client_socket) {
    char buffer[MAX_BUFFER];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, MAX_BUFFER - 1, 0);
        if (bytes_received <= 0) {
            printf("클라이언트 연결 종료\n");
            break;
        }

        buffer[bytes_received] = '\0';
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
            process_user_update(buffer, client_socket);
        }
        else if (strncmp(buffer, "BI/", 3) == 0) {
            process_insurance_info(buffer, client_socket);
        }
        else if (strncmp(buffer, "BS/", 3) == 0) {
            process_insurance_compensation_info(buffer, client_socket);
        }
        else if (strncmp(buffer, "AI/", 3) == 0) {
            process_accident_info(buffer, client_socket);
        }
        else if (strncmp(buffer, "LAW", 3) == 0) {
            LAW_guide(client_socket);
        }
    }

    closesocket(client_socket);
}

void process_login(char* buffer, SOCKET client_socket) {
    char* token;
    char id[MAX_INFO_LENGTH];
    char password[MAX_INFO_LENGTH];
    char response[MAX_BUFFER];

    // 프로토콜 파싱
    token = strtok(buffer, "/");
    token = strtok(NULL, "/");  // ID
    strcpy(id, token);
    token = strtok(NULL, "/");  // PW
    strcpy(password, token);

    // 사용자 인증
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, id) == 0 && strcmp(users[i].password, password) == 0) {
            sprintf(response, "SUCCESS/%s/%s/%s/%s", 
                    users[i].car_number, 
                    users[i].car_type, 
                    users[i].insurance_company,
                    users[i].id);
            send(client_socket, response, strlen(response), 0);
            printf("사용자 로그인 성공: %s\n", id);
            return;
        }
    }

    // 로그인 실패
    strcpy(response, "FAIL");
    send(client_socket, response, strlen(response), 0);
    printf("로그인 실패: %s\n", id);
}

void process_register(char* buffer, SOCKET client_socket) {
    char* token;
    char response[MAX_BUFFER];
    USER_DATA new_user;

    // 프로토콜 파싱
    token = strtok(buffer, "/");
    token = strtok(NULL, "/");  // ID
    strcpy(new_user.id, token);
    token = strtok(NULL, "/");  // PW
    strcpy(new_user.password, token);
    token = strtok(NULL, "/");  // 차량번호
    strcpy(new_user.car_number, token);
    token = strtok(NULL, "/");  // 차 종류
    strcpy(new_user.car_type, token);
    token = strtok(NULL, "/");  // 보험사
    strcpy(new_user.insurance_company, token);

    // ID 중복 체크
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, new_user.id) == 0) {
            strcpy(response, "FAIL/ID_EXISTS");
            send(client_socket, response, strlen(response), 0);
            return;
        }
    }

    // 새 사용자 추가
    users[user_count++] = new_user;
    save_user_data();

    strcpy(response, "회원가입 성공");
    send(client_socket, response, strlen(response), 0);
    printf("새 사용자 등록: %s\n", new_user.id);
}

void load_user_data() {
    FILE* file = fopen("C:\\Coding\\project\\project02\\users.txt", "r");
    if (file == NULL) {
        printf("사용자 데이터 파일을 열 수 없습니다.\n");
        return;
    }

    char line[MAX_BUFFER];
    while (fgets(line, sizeof(line), file) && user_count < MAX_USERS) {
        char* token = strtok(line, "/");
        strcpy(users[user_count].id, token);
        token = strtok(NULL, "/");
        strcpy(users[user_count].password, token);
        token = strtok(NULL, "/");
        strcpy(users[user_count].car_number, token);
        token = strtok(NULL, "/");
        strcpy(users[user_count].car_type, token);
        token = strtok(NULL, "/");
        strcpy(users[user_count].insurance_company, token);
        user_count++;
    }

    fclose(file);
}

void save_user_data() {
    FILE* file = fopen(USER_TEMP_FILE, "w");
    if (file == NULL) {
        printf("새로운 사용자 정보 파일을 열 수 없습니다.\n");
        return;
    }

    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s/%s/%s/%s/%s\n",
                users[i].id,
                users[i].password,
                users[i].car_number,
                users[i].car_type,
                users[i].insurance_company);
    }

    fclose(file);
}

void process_user_update(char* buffer, SOCKET client_socket) {
    char* token;
    char id[MAX_INFO_LENGTH];
    char update_type[MAX_INFO_LENGTH];
    char new_value[MAX_INFO_LENGTH];
    char response[MAX_BUFFER];

    // 프로토콜 파싱
    token = strtok(buffer, "/");
    token = strtok(NULL, "/");  // 업데이트 타입
    strcpy(update_type, token);
    token = strtok(NULL, "/");  // ID
    strcpy(id, token);
    token = strtok(NULL, "/");  // 새로운 값
    if (token != NULL) {
        strcpy(new_value, token);
    }

    // 사용자 찾기
    int user_index = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, id) == 0) {
            user_index = i;
            break;
        }
    }

    if (user_index == -1) {
        strcpy(response, "FAIL/USER_NOT_FOUND");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // 업데이트 타입에 따른 처리
    switch (update_type[0]) {
        case '1':  // 비밀번호 변경
            strcpy(users[user_index].password, new_value);
            break;
        case '2':  // 차 종류 변경
            strcpy(users[user_index].car_type, new_value);
            break;
        case '3':  // 차량번호 변경
            strcpy(users[user_index].car_number, new_value);
            break;
        case '4':  // 보험사 변경
            strcpy(users[user_index].insurance_company, new_value);
            break;
        case '5':  // 사용자 삭제
            for (int i = user_index; i < user_count - 1; i++) {
                users[i] = users[i + 1];
            }
            user_count--;
            break;
    }

    save_user_data();
    strcpy(response, "SUCCESS");
    send(client_socket, response, strlen(response), 0);
}

void process_insurance_info(char* buffer, SOCKET client_socket) {
    char* token;
    char ins_company[MAX_INFO_LENGTH];
    char info_type[MAX_INFO_LENGTH];
    char response[MAX_BUFFER * 10] = ""; // 여러 줄 안내 대비
    char filename[256];
    char line[MAX_BUFFER];
    FILE* file;

    // 프로토콜 파싱: BI/보험사명/항목명
    token = strtok(buffer, "/"); // "BI"
    token = strtok(NULL, "/");   // 보험사명
    if (token) strcpy(ins_company, token); else ins_company[0] = '\0';
    token = strtok(NULL, "/");   // 항목명
    if (token) strcpy(info_type, token); else info_type[0] = '\0';

    // 보험사별 파일명 결정
    if (strcmp(ins_company, "삼성화재") == 0) strcpy(filename, SAMSUNG_ACCIDENT_FILE);
    else if (strcmp(ins_company, "DB손해보험") == 0) strcpy(filename, DB_ACCIDENT_FILE);
    else if (strcmp(ins_company, "한화손해보험") == 0) strcpy(filename, HANWHA_ACCIDENT_FILE);
    else if (strcmp(ins_company, "KB손해보험") == 0) strcpy(filename, KB_ACCIDENT_FILE);
    else {
        strcpy(response, "지원하지 않는 보험사입니다.");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    file = fopen(filename, "r");
    if (!file) {
        strcpy(response, "보험사 파일을 열 수 없습니다.");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // 항목명 포함된 줄부터 빈 줄까지 추출
    int in_section = 0;
    response[0] = '\0';
    while (fgets(line, sizeof(line), file)) {
        if (!in_section) {
            // 예: "1. 대인배상" 등에서 info_type이 포함된 줄 찾기
            if (strstr(line, info_type)) {
                in_section = 1;
                strcat(response, line);
            }
            continue;
        }
        if (in_section) {
            // 빈 줄(\n, \r\n)에서 종료
            if (strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) break;
            strcat(response, line);
        }
    }
    fclose(file);

    if (strlen(response) == 0) strcpy(response, "해당 항목 안내가 없습니다.");
    send(client_socket, response, strlen(response), 0);
}

void process_accident_info(char* buffer, SOCKET client_socket) {
    char* token;
    ACCIDENT_DATA new_accident;
    char response[MAX_BUFFER];

    // 프로토콜 파싱: AI/ID/언제/무엇을/어떻게/가해자or피해자
    token = strtok(buffer, "/"); // "AI"
    token = strtok(NULL, "/");   // ID
    if (token) strcpy(new_accident.id, token); else new_accident.id[0] = '\0';
    token = strtok(NULL, "/");   // 언제
    if (token) strcpy(new_accident.accident_date, token); else new_accident.accident_date[0] = '\0';
    token = strtok(NULL, "/");   // 무엇을
    if (token) strcpy(new_accident.accident_type, token); else new_accident.accident_type[0] = '\0';
    token = strtok(NULL, "/");   // 어떻게
    if (token) strcpy(new_accident.accident_description, token); else new_accident.accident_description[0] = '\0';
    token = strtok(NULL, "/");   // 가해자or피해자
    if (token) strcpy(new_accident.accident_role, token); else new_accident.accident_role[0] = '\0';

    // 사고 정보 저장
    if (accident_count < MAX_ACCIDENTS) {
        accidents[accident_count++] = new_accident;
        save_accident_data();
        strcpy(response, "SUCCESS");
    } else {
        strcpy(response, "FAIL/ACCIDENT_FULL");
    }
    send(client_socket, response, strlen(response), 0);
}

void load_accident_data() {
    FILE* file = fopen(ACCIDENT_FILE, "r");
    if (file == NULL) {
        printf("사고 데이터 파일을 열 수 없습니다.\n");
        return;
    }

    char line[MAX_BUFFER];
    while (fgets(line, sizeof(line), file) && accident_count < MAX_ACCIDENTS) {
        char* token = strtok(line, "/");
        strcpy(accidents[accident_count].id, token);
        token = strtok(NULL, "/");
        strcpy(accidents[accident_count].accident_date, token);
        token = strtok(NULL, "/");
        strcpy(accidents[accident_count].accident_type, token);
        token = strtok(NULL, "/");
        strcpy(accidents[accident_count].accident_description, token);
        token = strtok(NULL, "/");
        strcpy(accidents[accident_count].accident_role, token);
        accident_count++;
    }

    fclose(file);
}

void save_accident_data() {
    FILE* file = fopen(ACCIDENT_FILE, "w");
    if (file == NULL) {
        printf("사고 데이터 파일을 열 수 없습니다.\n");
        return;
    }

    for (int i = 0; i < accident_count; i++) {
        fprintf(file, "%s/%s/%s/%s/%s\n",
                accidents[i].id,
                accidents[i].accident_date,
                accidents[i].accident_type,
                accidents[i].accident_description,
                accidents[i].accident_role);
    }

    fclose(file);
}

void accident_data(SOCKET sock, const char* id) {
    char when[100], what[100], how[200], role[20];
    char buffer[MAX_BUFFER];

    printf("사고 정보 입력\n");
    printf("언제: ");
    fgets(when, sizeof(when), stdin); when[strcspn(when, "\r\n")] = '\0';

    printf("무엇을: ");
    fgets(what, sizeof(what), stdin); what[strcspn(what, "\r\n")] = '\0';

    printf("어떻게: ");
    fgets(how, sizeof(how), stdin); how[strcspn(how, "\r\n")] = '\0';

    printf("가해자or피해자: ");
    fgets(role, sizeof(role), stdin); role[strcspn(role, "\r\n")] = '\0';

    // 큐형식으로 입력받은 정보를 서버에 전송
    snprintf(buffer, sizeof(buffer), "AI/%s/%s/%s/%s/%s", id, when, what, how, role);
    send(sock, buffer, strlen(buffer), 0);

    // 서버 응답 수신
    int len = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("서버 응답: %s\n", buffer);
    }
}

void process_insurance_compensation_info(char* buffer, SOCKET client_socket) {
    char* token;
    char ins_company[MAX_INFO_LENGTH];
    char info_type[MAX_INFO_LENGTH];
    char response[MAX_BUFFER * 10] = "";
    char filename[256];
    char line[MAX_BUFFER];
    FILE* file;

    // 프로토콜 파싱: BS/보험사명/항목명
    token = strtok(buffer, "/"); // "BS"
    token = strtok(NULL, "/");   // 보험사명
    if (token) strcpy(ins_company, token); else ins_company[0] = '\0';
    token = strtok(NULL, "/");   // 항목명
    if (token) strcpy(info_type, token); else info_type[0] = '\0';

    // 보험사별 보상안내 파일명 결정
    if (strcmp(ins_company, "삼성화재") == 0) strcpy(filename, SAMSUNG_CPS_FILE);
    else if (strcmp(ins_company, "DB손해보험") == 0) strcpy(filename, DB_CPS_FILE);
    else if (strcmp(ins_company, "한화손해보험") == 0) strcpy(filename, HANWHA_CPS_FILE);
    else if (strcmp(ins_company, "KB손해보험") == 0) strcpy(filename, KB_CPS_FILE);
    else {
        strcpy(response, "지원하지 않는 보험사입니다.");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    file = fopen(filename, "r");
    if (!file) {
        strcpy(response, "보험사 파일을 열 수 없습니다.");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // 항목명 포함된 줄부터 빈 줄까지 추출
    int in_section = 0;
    response[0] = '\0';
    while (fgets(line, sizeof(line), file)) {
        if (!in_section) {
            if (strstr(line, info_type)) {
                in_section = 1;
                strcat(response, line);
            }
            continue;
        }
        if (in_section) {
            if (strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) break;
            strcat(response, line);
        }
    }
    fclose(file);
    if (strlen(response) == 0) strcpy(response, "해당 항목 안내가 없습니다.");
    send(client_socket, response, strlen(response), 0);
}

void LAW_guide(SOCKET client_socket) {
    char buffer[MAX_BUFFER * 10];
    FILE* file = fopen(ACCIDENT_LAW_FILE, "r");
    if (!file) {
        strcpy(buffer, "법률 안내 파일을 열 수 없습니다.");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }
    buffer[0] = '\0';
    char line[MAX_BUFFER];
    while (fgets(line, sizeof(line), file)) {
        strcat(buffer, line);
    }
    fclose(file);
    send(client_socket, buffer, strlen(buffer), 0);
}

int handle_login(const char* id, const char* pw) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, id) == 0 && strcmp(users[i].password, pw) == 0) {
            return 1; // 로그인 성공
        }
    }
    return 0; // 로그인 실패
}

int handle_register(const char* id, const char* pw, const char* car_number, const char* car_type, const char* ins_company) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, id) == 0) {
            return 0; // ID 중복
        }
    }
    if (user_count >= MAX_USERS) return 0;
    strcpy(users[user_count].id, id);
    strcpy(users[user_count].password, pw);
    strcpy(users[user_count].car_number, car_number);
    strcpy(users[user_count].car_type, car_type);
    strcpy(users[user_count].insurance_company, ins_company);
    user_count++;
    save_user_data();
    return 1;
}










