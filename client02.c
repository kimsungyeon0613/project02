#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 1024
#define MAX_USERS 100
#define MAX_ACCIDENTS 100
#define MAX_INFO_LENGTH 30

// 사용자 정보 구조체
typedef struct {
    char id[MAX_INFO_LENGTH];
    char password[MAX_INFO_LENGTH];
    char car_number[MAX_INFO_LENGTH];
    char car_type[MAX_INFO_LENGTH];
    char insurance_company[MAX_INFO_LENGTH];
} USER_DATA;

// 사고 정보 구조체
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
void process_login(char* buffer, SOCKET client_socket);  // 로그인 처리
void process_register(char* buffer, SOCKET client_socket);  // 회원가입 처리
void process_user_update(char* buffer, SOCKET client_socket);  // 사용자 정보 수정
void accident_info(char* buffer, SOCKET client_socket);  // 사고 정보
void accident_data(char* buffer, SOCKET client_socket);  // 사고 유형별 안내
void accident_inscompany(char* buffer, SOCKET client_socket);  // 보험사별 안내
void save_user_data();  // 사용자 데이터 저장
void load_accident_data();  // 사고 데이터 로드
void save_accident_data();  // 사고 데이터 저장
void 

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

    // 서버 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("소켓 생성 실패\n");
        return 1;
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9000);

    // 소켓 바인딩
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("바인딩 실패\n");
        return 1;
    }

    // 연결 대기
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen 실패\n");
        return 1;
    }

    printf("서버가 시작되었습니다. 포트: 9000\n");

    // 사용자 데이터 로드
    load_user_data();
    load_accident_data();

    while (1) {
        client_addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
        
        if (client_socket == INVALID_SOCKET) {
            printf("클라이언트 연결 실패\n");
            continue;
        }

        printf("클라이언트 연결됨: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        // 클라이언트 처리
        handle_client(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
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

        // 프로토콜 처리
        if (strncmp(buffer, "lo/", 3) == 0) {
            process_login(buffer, client_socket);
        }
        else if (strncmp(buffer, "new/", 4) == 0) {
            process_register(buffer, client_socket);
        }
        else if (strncmp(buffer, "UF/", 3) == 0) {
            process_user_update(buffer, client_socket);
        }
        else if (strncmp(buffer, "BI/", 3) == 0) {
            process_insurance_info(buffer, client_socket);
        }
        else if (strncmp(buffer, "BS/", 3) == 0) {
            process_insurance_info(buffer, client_socket);
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

    strcpy(response, "SUCCESS");
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
    FILE* file = fopen("C:\\Coding\\project\\project02\\users.txt", "w");
    if (file == NULL) {
        printf("사용자 데이터 파일을 열 수 없습니다.\n");
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
    char id[MAX_INFO_LENGTH];
    char info_type[MAX_INFO_LENGTH];
    char response[MAX_BUFFER];
    char insurance_file[MAX_BUFFER];
    char line[MAX_BUFFER];
    FILE* file;

    // 프로토콜 파싱
    token = strtok(buffer, "/");
    token = strtok(NULL, "/");  // 정보 타입
    strcpy(info_type, token);
    token = strtok(NULL, "/");  // ID
    strcpy(id, token);

    // 사용자의 보험사 찾기
    char insurance_company[MAX_INFO_LENGTH] = "";
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, id) == 0) {
            strcpy(insurance_company, users[i].insurance_company);
            break;
        }
    }

    if (strlen(insurance_company) == 0) {
        strcpy(response, "FAIL/USER_NOT_FOUND");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // 보험사 파일 이름 설정
    if (strcmp(insurance_company, "삼성화재") == 0) {
        strcpy(insurance_file, "samsung.txt");
    }
    else if (strcmp(insurance_company, "DB손해보험") == 0) {
        strcpy(insurance_file, "db.txt");
    }
    else if (strcmp(insurance_company, "한화손해보험") == 0) {
        strcpy(insurance_file, "hanwha.txt");
    }
    else if (strcmp(insurance_company, "KB손해보험") == 0) {
        strcpy(insurance_file, "kb.txt");
    }

    // 보험사 정보 파일 읽기
    file = fopen(insurance_file, "r");
    if (file == NULL) {
        strcpy(response, "FAIL/FILE_NOT_FOUND");
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // 요청된 정보 찾기
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, info_type) != NULL) {
            found = 1;
            strcpy(response, "SUCCESS/");
            strcat(response, line);
            send(client_socket, response, strlen(response), 0);
            break;
        }
    }

    if (!found) {
        strcpy(response, "FAIL/INFO_NOT_FOUND");
        send(client_socket, response, strlen(response), 0);
    }

    fclose(file);
}

void process_accident_info(char* buffer, SOCKET client_socket) {
    char* token;
    char id[MAX_INFO_LENGTH];
    char response[MAX_BUFFER];
    ACCIDENT_DATA new_accident;

    // 프로토콜 파싱
    token = strtok(buffer, "/");
    token = strtok(NULL, "/");  // ID
    strcpy(new_accident.id, token);
    token = strtok(NULL, "/");  // 사고 날짜
    strcpy(new_accident.accident_date, token);
    token = strtok(NULL, "/");  // 사고 유형
    strcpy(new_accident.accident_type, token);
    token = strtok(NULL, "/");  // 사고 설명
    strcpy(new_accident.accident_description, token);
    token = strtok(NULL, "/");  // 가해자/피해자
    strcpy(new_accident.accident_role, token);

    // 사고 정보 저장
    accidents[accident_count++] = new_accident;
    save_accident_data();

    strcpy(response, "SUCCESS");
    send(client_socket, response, strlen(response), 0);
}

void load_accident_data() {
    FILE* file = fopen("C:\\Coding\\project\\project02\\accident.txt", "r");
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
    FILE* file = fopen("C:\\Coding\\project\\project02\\accident.txt", "w");
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