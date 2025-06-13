#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <ctype.h>
#include <errno.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP   "220.89.224.138"
#define SERVER_PORT 5548
#define BUF_SIZE    1000
#define ACCIDENT_LAW_FILE "C:\\Coding\\project\\program02\\accident_law.txt"

int login(SOCKET sock, char* user_ins_company);
void show_accident_guide(SOCKET sock, const char* user_ins_company);

void ErrorHandling(const char* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

// 회원가입 함수
int register_user(SOCKET sock) {
    char buffer[BUF_SIZE];
    char id[20], pw[20], car_number[20], car_type[20], ins_company[20];
    int car_choice, ins_choice;
    
    printf("\n=== 회원가입 ===\n");
    
    // ID 입력
    printf("ID를 입력하세요: ");
    if (!fgets(id, sizeof(id), stdin))
        ErrorHandling("입력 오류");
    id[strcspn(id, "\r\n")] = '\0';
    
    // PW 입력
    printf("비밀번호를 입력하세요: ");
    if (!fgets(pw, sizeof(pw), stdin))
        ErrorHandling("입력 오류");
    pw[strcspn(pw, "\r\n")] = '\0';
    
    // 차량번호 입력
    printf("차량번호를 입력하세요: ");
    if (!fgets(car_number, sizeof(car_number), stdin))
        ErrorHandling("입력 오류");
    car_number[strcspn(car_number, "\r\n")] = '\0';
    
    // 차종류 입력
    while (1) {
        printf("\n차종류 선택하세요(숫자).\n");
        printf("1. SUV\n");
        printf("2. 승용차\n");
        printf("3. 트럭\n");
        printf("선택: ");
        scanf("%d", &car_choice);
        getchar(); // 버퍼 비우기
        
        if (car_choice >= 1 && car_choice <= 3) break;
        printf("잘못된 선택입니다. 다시 선택해주세요.\n");
    }
    
    switch(car_choice) {
        case 1: strcpy(car_type, "SUV"); break;
        case 2: strcpy(car_type, "승용차"); break;
        case 3: strcpy(car_type, "트럭"); break;
    }
    
    // 보험사 입력
    while (1) {
        printf("\n보험사 선택(숫자)\n");
        printf("1. 삼성화재\n");
        printf("2. 한화손해보험\n");
        printf("3. KB손해보험\n");
        printf("4. DB손해보험\n");
        printf("선택: ");
        scanf("%d", &ins_choice);
        getchar(); // 버퍼 비우기
        
        if (ins_choice >= 1 && ins_choice <= 4) break;
        printf("잘못된 선택입니다. 다시 선택해주세요.\n");
    }
    
    switch(ins_choice) {
        case 1: strcpy(ins_company, "삼성화재"); break;
        case 2: strcpy(ins_company, "한화손해보험"); break;
        case 3: strcpy(ins_company, "KB손해보험"); break;
        case 4: strcpy(ins_company, "DB손해보험"); break;
    }
    
    // 입력 정보 확인
    printf("\n=== 입력 정보 확인 ===\n");
    printf("ID: %s\n", id);
    printf("차량번호: %s\n", car_number);
    printf("차종류: %s\n", car_type);
    printf("보험사: %s\n", ins_company);
    printf("\n회원가입을 진행하시겠습니까? (1:예, 2:아니오): ");
    
    int confirm;
    scanf("%d", &confirm);
    getchar(); // 버퍼 비우기
    
    if (confirm != 1) {
        printf("회원가입이 취소되었습니다.\n");
        return 0;
    }
    
    // 회원가입 요청 전송
    sprintf(buffer, "new/%s/%s/%s/%s/%s", id, pw, car_number, car_type, ins_company);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");
    
    // 서버 응답 수신
    int strLen = recv(sock, buffer, BUF_SIZE, 0);
    if (strLen == SOCKET_ERROR)
        ErrorHandling("recv() error");
    buffer[strLen] = '\0';
    
    if (strcmp(buffer, "1") == 0) {
        printf("\n회원가입이 완료되었습니다.\n");
        printf("로그인을 진행합니다.\n");
        return 1;
    } else {
        printf("\n회원가입 실패! (ID 중복)\n");
        return 0;
    }
}

// 사용자 정보 수정 함수
void update_user_info(SOCKET sock, const char* user_id) {
    char update_type[2];
    char new_value[256];
    int choice;

    while (1) {
        printf("\n=== 사용자 정보 수정 ===\n");
        printf("1. 비밀번호 변경\n");
        printf("2. 차번호 변경\n");
        printf("3. 차종류 변경\n");
        printf("4. 보험사 변경\n");
        printf("5. 전체 정보 수정\n");
        printf("6. 이전 메뉴로\n");
        printf("선택: ");
        scanf("%d", &choice);
        getchar(); // 버퍼 비우기

        switch (choice) {
            case 1: // 비밀번호 변경
                printf("새 비밀번호: ");
                if (!fgets(new_value, sizeof(new_value), stdin))
                    ErrorHandling("입력 오류");
                new_value[strcspn(new_value, "\r\n")] = '\0';
                sprintf(update_type, "1");
                break;

            case 2: // 차번호 변경
                printf("새 차번호: ");
                if (!fgets(new_value, sizeof(new_value), stdin))
                    ErrorHandling("입력 오류");
                new_value[strcspn(new_value, "\r\n")] = '\0';
                sprintf(update_type, "2");
                break;

            case 3: // 차종류 변경
                printf("\n=== 차종류 선택 ===\n");
                printf("1. SUV\n");
                printf("2. 승용차\n");
                printf("3. 트럭\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar(); // 버퍼 비우기

                switch (choice) {
                    case 1:
                        strcpy(new_value, "SUV");
                        break;
                    case 2:
                        strcpy(new_value, "승용차");
                        break;
                    case 3:
                        strcpy(new_value, "트럭");
                        break;
                    default:
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                sprintf(update_type, "3");
                break;

            case 4: // 보험사 변경
                printf("\n=== 보험사 선택 ===\n");
                printf("1. 삼성화재\n");
                printf("2. 한화손해보험\n");
                printf("3. KB손해보험\n");
                printf("4. DB손해보험\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar(); // 버퍼 비우기

                switch (choice) {
                    case 1:
                        strcpy(new_value, "삼성화재");
                        break;
                    case 2:
                        strcpy(new_value, "한화손해보험");
                        break;
                    case 3:
                        strcpy(new_value, "KB손해보험");
                        break;
                    case 4:
                        strcpy(new_value, "DB손해보험");
                        break;
                    default:
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                sprintf(update_type, "4");
                break;

            case 5: // 전체 정보 수정
                printf("새 비밀번호: ");
                if (!fgets(new_value, sizeof(new_value), stdin))
                    ErrorHandling("입력 오류");
                new_value[strcspn(new_value, "\r\n")] = '\0';
                strcat(new_value, "\n");

                printf("새 차번호: ");
                char temp[256];
                if (!fgets(temp, sizeof(temp), stdin))
                    ErrorHandling("입력 오류");
                temp[strcspn(temp, "\r\n")] = '\0';
                strcat(new_value, temp);
                strcat(new_value, "\n");

                printf("\n=== 차종류 선택 ===\n");
                printf("1. SUV\n");
                printf("2. 승용차\n");
                printf("3. 트럭\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar(); // 버퍼 비우기

                switch (choice) {
                    case 1:
                        strcat(new_value, "SUV");
                        break;
                    case 2:
                        strcat(new_value, "승용차");
                        break;
                    case 3:
                        strcat(new_value, "트럭");
                        break;
                    default:
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                strcat(new_value, "\n");

                printf("\n=== 보험사 선택 ===\n");
                printf("1. 삼성화재\n");
                printf("2. 한화손해보험\n");
                printf("3. KB손해보험\n");
                printf("4. DB손해보험\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar(); // 버퍼 비우기

                switch (choice) {
                    case 1:
                        strcat(new_value, "삼성화재");
                        break;
                    case 2:
                        strcat(new_value, "한화손해보험");
                        break;
                    case 3:
                        strcat(new_value, "KB손해보험");
                        break;
                    case 4:
                        strcat(new_value, "DB손해보험");
                        break;
                    default:
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                sprintf(update_type, "5");
                break;

            case 6: // 이전 메뉴로
                return;

            default:
                printf("잘못된 선택입니다.\n");
                continue;
        }

        // 서버에 수정 요청 전송
        char request[512];
        sprintf(request, "UF/%s/%s/%s", update_type, user_id, new_value);
        send(sock, request, strlen(request), 0);

        // 서버 응답 수신
        char response[2];
        if (recv(sock, response, 1, 0) <= 0)
            ErrorHandling("서버 응답 수신 실패");

        if (response[0] == '1')
            printf("정보가 성공적으로 수정되었습니다.\n");
        else
            printf("정보 수정에 실패했습니다.\n");
    }
}

// 초기 메뉴 함수
void show_initial_menu(SOCKET sock, char* user_ins_company) {
    while (1) {
        printf("\n=== 교통사고 처리 안내 시스템 ===\n");
        printf("1. 로그인\n");
        printf("2. 회원가입\n");
        printf("3. 종료\n");
        printf("선택: ");
        
        int choice;
        scanf("%d", &choice);
        getchar(); // 버퍼 비우기
        
        switch (choice) {
            case 1: // 로그인
                if (login(sock, user_ins_company)) {
                    return; // 로그인 성공 시 함수 종료
                }
                break;
                
            case 2: // 회원가입
                if (register_user(sock)) {
                    // 회원가입 성공 후 로그인 화면으로 자동 전환
                    if (login(sock, user_ins_company)) {
                        return;
                    }
                }
                break;
                
            case 3: // 종료
                printf("\n프로그램을 종료합니다.\n");
                closesocket(sock);
                WSACleanup();
                exit(0);
                
            default:
                printf("\n잘못된 선택입니다. 다시 입력하세요!\n");
        }
    }
}

void accident_data(SOCKET sock, const char* id) {
    char when[20], what[20], how[200], role[20];
    char buffer[BUF_SIZE];

    printf("\n=== 사고 정보 입력 ===\n");
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
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
        ErrorHandling("사고 정보 전송 오류");
    }

    // 서버 응답 수신
    int len = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("서버 응답: %s\n", buffer);
    } else {
        printf("서버 응답 수신 실패\n");
    }
}

void LAW_data(SOCKET client_socket) {
    char buffer[BUF_SIZE];
    // 서버에 LAW 명령어 전송
    strcpy(buffer, "LAW");
    if (send(client_socket, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");
    // 서버로부터 12대 중과실 법률 안내 수신
    int strLen = recv(client_socket, buffer, BUF_SIZE - 1, 0);
    if (strLen == SOCKET_ERROR)
        ErrorHandling("recv() error");
    buffer[strLen] = '\0';
    printf("\n[12대 중과실 법률 안내]\n%s\n", buffer);
}

void show_accident_guide(SOCKET sock, const char* user_ins_company) {
    int choice;
    const char* type_names[] = {"", "대인배상", "대물배상", "자기신체사고", "자동차상해", "자기차량손해"};
    printf("\n================ 보험사 사고 유형별 안내 메뉴 ================\n");
    printf(" 1. 대인배상\n 2. 대물배상\n 3. 자기신체사고\n 4. 자동차상해\n 5. 자기차량손해\n");
    printf("============================================================\n");
    printf("원하는 메뉴(번호)를 선택하세요: ");
    scanf("%d", &choice); getchar();

    if (choice < 1 || choice > 5) {
        printf("잘못된 선택입니다.\n");
        return;
    }

    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "BI/%s/%s", user_ins_company, type_names[choice]);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
        ErrorHandling("send() error");
    }

    // 서버 응답 수신 및 출력
    int len = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("\n[안내]\n%s\n", buffer);
    } else {
        printf("서버 응답 수신 실패\n");
    }
}

// 보험사 보상안내 인터페이스 함수 추가
void show_compensation_guide(SOCKET sock, const char* user_ins_company) {
    int choice;
    const char* type_names[] = {"", "대인배상", "대물배상", "자기신체사고", "자동차상해", "자기차량손해"};
    printf("\n================ 보험사 보상 안내 메뉴 ================\n");
    printf(" 1. 대인배상\n 2. 대물배상\n 3. 자기신체사고\n 4. 자동차상해\n 5. 자기차량손해\n");
    printf("====================================================\n");
    printf("원하는 메뉴(번호)를 선택하세요: ");
    scanf("%d", &choice); getchar();

    if (choice < 1 || choice > 5) {
        printf("잘못된 선택입니다.\n");
        return;
    }

    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "BS/%s/%s", user_ins_company, type_names[choice]);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
        ErrorHandling("send() error");
    }

    // 서버 응답 수신 및 출력
    int len = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("\n[보상 안내]\n%s\n", buffer);
    } else {
        printf("서버 응답 수신 실패\n");
    }
}

// 메인 메뉴 함수
void show_main_menu(SOCKET sock, const char* user_ins_company) {
    while (1) {
        printf("\n=== 메인 메뉴 ===\n");
        printf("1. 사용자 정보 수정\n");
        printf("2. 사고 정보 입력\n");
        printf("3. 사고 유형별 안내\n");
        printf("4. 보험사 보상 안내\n");
        printf("5. 12대 중과실 법률 안내\n");
        printf("6. 사고대처 안내\n");
        printf("7. 프로그램 종료\n");
        printf("선택: ");
        
        int choice;
        scanf("%d", &choice);
        getchar(); // 버퍼 비우기
        
        switch (choice) {
            case 1: // 사용자 정보 수정
                update_user_info(sock, user_ins_company);
                break;
            case 2: //사고 정보 입력
                accident_data(sock, user_ins_company);
                break;
            case 3: // 사고 유형별 안내
                show_accident_guide(sock, user_ins_company);
                break;
            case 4:
                show_compensation_guide(sock, user_ins_company);
                break;
            case 5: // 12대 중과실 법률 안내
                LAW_data(sock);
                break;
            case 6:
                printf("사고대처 안내는 추후 구현 예정입니다.\n");
                break;
            case 7: // 프로그램 종료
                printf("프로그램을 종료합니다.\n");
                closesocket(sock);
                WSACleanup();
                exit(0);
            default:
                printf("\n잘못된 선택입니다.\n");
        }
    }
}

// 로그인 함수 구현 추가
int login(SOCKET sock, char* user_ins_company) {
    char id[20], pw[20], buffer[BUF_SIZE];
    printf("\n=== 로그인 ===\n");
    printf("ID: ");
    if (!fgets(id, sizeof(id), stdin)) ErrorHandling("입력 오류");
    id[strcspn(id, "\r\n")] = '\0';
    printf("비밀번호: ");
    if (!fgets(pw, sizeof(pw), stdin)) ErrorHandling("입력 오류");
    pw[strcspn(pw, "\r\n")] = '\0';
    sprintf(buffer, "lo/%s/%s", id, pw);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");
    int strLen = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (strLen == SOCKET_ERROR)
        ErrorHandling("recv() error");
    buffer[strLen] = '\0';
    if (strcmp(buffer, "0") == 0 || strcmp(buffer, "FAIL") == 0) {
        printf("로그인 실패!\n");
        return 0;
    } else {
        // 서버에서 보험사명 등 추가 정보가 오면 파싱해서 저장
        // 예: SUCCESS/차량번호/차종류/보험사/ID
        char* token = strtok(buffer, "/"); // SUCCESS
        token = strtok(NULL, "/"); // 차량번호
        token = strtok(NULL, "/"); // 차종류
        token = strtok(NULL, "/"); // 보험사
        if (token) strcpy(user_ins_company, token);
        printf("로그인 성공!\n");
        return 1;
    }
}

int main(void) {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in servAddr;
    char buffer[BUF_SIZE];
    int strLen;
    char user_ins_company[32] = "";
    SetConsoleOutputCP(CP_UTF8);
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error");
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        ErrorHandling("socket() error");
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servAddr.sin_port        = htons(SERVER_PORT);
    if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        ErrorHandling("connect() error");
    fputs("서버에 연결되었습니다.\n\n", stdout);
    show_initial_menu(sock, user_ins_company);
    show_main_menu(sock, user_ins_company);
    closesocket(sock);
    WSACleanup();
    return 0;
}