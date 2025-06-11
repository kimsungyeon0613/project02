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
        printf("\n차종류 선택하세요.\n");
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
        printf("\n보험사 선택\n");
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
void show_initial_menu(SOCKET sock) {
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
                if (login(sock)) {
                    return; // 로그인 성공 시 함수 종료
                }
                break;
                
            case 2: // 회원가입
                if (register_user(sock)) {
                    // 회원가입 성공 후 로그인 화면으로 자동 전환
                    if (login(sock)) {
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
                printf("\n잘못된 선택입니다.\n");
        }
    }
}

// 메인 메뉴 함수
void show_main_menu(SOCKET sock, const char* user_id) {
    while (1) {
        printf("\n=== 메인 메뉴 ===\n");
        printf("1. 사용자 정보 수정\n");
        printf("2. 사고 유형별 안내\n");
        printf("3. 사고 정보 입력\n");
        printf("4. 보험사 보상 안내\n");
        printf("5. 로그아웃\n");
        printf("선택: ");
        
        int choice;
        scanf("%d", &choice);
        getchar(); // 버퍼 비우기
        
        switch (choice) {
            case 1: // 사용자 정보 수정
                update_user_info(sock, user_id);
                break;
                
            case 2: // 사고 유형별 안내
                show_accident_guide(sock, user_id);
                break;
                
            case 3: // 사고 정보 입력
                // TODO: 사고 정보 입력 기능 구현
                printf("사고 정보 입력 기능은 아직 구현되지 않았습니다.\n");
                break;
                
            case 4: // 보험사 보상 안내
                // TODO: 보험사 보상 안내 기능 구현
                printf("보험사 보상 안내 기능은 아직 구현되지 않았습니다.\n");
                break;
                
            case 5: // 로그아웃
                return;
                
            default:
                printf("\n잘못된 선택입니다.\n");
        }
    }
}

int main(void) {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in servAddr;
    char buffer[BUF_SIZE];
    int strLen;

    char id[20], pw[20], cmd[BUF_SIZE];

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
    
    // 초기 메뉴 표시
    show_initial_menu(sock);

  for (;;) {
        fputs("\n메뉴:\n", stdout);
        fputs("1. 사용자 정보 수정\n", stdout);
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
            printf("1번. 비밀번호 변경");
            printf("2번. 차 번호 수정");
            printf("3번. 차종류 수정");
            printf("4번. 보험사 수정");
            printf("5번. 전체정보 수정");
            
            fputs(": ", stdout); fflush(stdout);
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

    closesocket(sock);
    WSACleanup();
    return 0;
}