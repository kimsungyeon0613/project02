#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h> 
#include <locale.h>

#pragma comment(lib, "ws2_32.lib")


#define SERVER_PORT 5050
#define BUF_SIZE    1024

void ErrorHandling(const char* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}


void get_password_securely(char* buffer, int buffer_size) {
    int i = 0;
    char ch;
    while (1) {
        ch = _getch(); 

        if (ch == 13) { 
            break;
        } else if (ch == 8) { 
            if (i > 0) {
                i--;
                printf("\b \b"); 
            }
        } else if (i < buffer_size - 1) { 
            buffer[i++] = ch;
            printf("*");
        }
    }
    buffer[i] = '\0'; 
    printf("\n");
}


int login(SOCKET sock, char* logged_in_user_id) {
    char id[20], pw[20];
    char buffer[BUF_SIZE];
    
    printf("\n=== 로그인 ===\n");
    printf("ID: ");
    if (!fgets(id, sizeof(id), stdin))
        ErrorHandling("입력 오류");
    id[strcspn(id, "\r\n")] = '\0';
    
    printf("비밀번호: ");
    get_password_securely(pw, sizeof(pw));

    sprintf(buffer, "lo/%s/%s", id, pw);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");
    int strLen = recv(sock, buffer, BUF_SIZE, 0);
    if (strLen == SOCKET_ERROR)
        ErrorHandling("recv() error");
    buffer[strLen] = '\0';
    
    printf("%s", buffer);
    

    if (strncmp(buffer, "OK:", 3) == 0) {
        printf("로그인이 성공적으로 완료되었습니다.\n");
        strncpy(logged_in_user_id, id, 19);
        logged_in_user_id[19] = '\0';
        return 1;
    } else {
        printf("로그인에 실패했습니다.\n");
        return 0;
    }
}

// 회원가입 함수
int new_user(SOCKET sock) {
    char buffer[BUF_SIZE];
    char id[20], pw[20], car_number[20], car_type[20], ins_company[20];
    int car_choice, ins_choice;
    
    printf("\n=== 회원가입 ===\n");
    printf("(입력 중 '취소'를 입력하면 이전 메뉴로 돌아갑니다)\n");
    
    while (1) {
        printf("ID를 입력하세요: ");
        if (!fgets(id, sizeof(id), stdin))
            ErrorHandling("입력 오류");
        id[strcspn(id, "\r\n")] = '\0';

        if (strcmp(id, "취소") == 0 || strcmp(id, "cancel") == 0) {
            printf("회원가입을 취소합니다.\n");
            return 0;
        }

        sprintf(buffer, "check_id/%s", id);
        if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
            ErrorHandling("send() error");
 
        int strLen = recv(sock, buffer, BUF_SIZE, 0);
        if (strLen == SOCKET_ERROR)
            ErrorHandling("recv() error");
        buffer[strLen] = '\0';
        
        if (strcmp(buffer, "0\n") == 0) {

            break;
        } else {
            printf("중복된 ID가 있습니다. 다른 ID를 입력해주세요.\n");
        }
    }

    printf("비밀번호를 입력하세요: ");
    get_password_securely(pw, sizeof(pw));
    
    printf("차량번호를 입력하세요: ");
    if (!fgets(car_number, sizeof(car_number), stdin))
        ErrorHandling("입력 오류"); 
    car_number[strcspn(car_number, "\r\n")] = '\0';

    if (strcmp(car_number, "취소") == 0 || strcmp(car_number, "cancel") == 0) {
        printf("회원가입을 취소합니다.\n");
        return 0;
    }

    while (1) {
        printf("\n차종류 선택하세요.\n");
        printf("1. SUV\n");
        printf("2. 승용차\n");
        printf("3. 대형차\n");
        printf("4. 이전 메뉴로\n");
        printf("선택: ");
        scanf("%d", &car_choice);
        getchar(); 
        
        if (car_choice == 4) {
            printf("회원가입을 취소합니다.\n");
            return 0;
        }
        
        if (car_choice >= 1 && car_choice <= 3) break;
        printf("잘못된 선택입니다. 다시 선택해주세요.\n");
    }
    
    switch(car_choice) {
        case 1: strcpy(car_type, "SUV"); break;
        case 2: strcpy(car_type, "승용차"); break;
        case 3: strcpy(car_type, "대형차"); break;
    }

    while (1) {
        printf("\n보험사 선택\n");
        printf("1. 삼성화재\n");
        printf("2. 한화손해보험\n");
        printf("3. KB손해보험\n");
        printf("4. DB손해보험\n");
        printf("5. 이전 메뉴로\n");
        printf("선택: ");
        scanf("%d", &ins_choice);
        getchar(); 
        
        if (ins_choice == 5) {
            printf("회원가입을 취소합니다.\n");
            return 0;
        }
        
        if (ins_choice >= 1 && ins_choice <= 4) break;
        printf("잘못된 선택입니다. 다시 선택해주세요.\n");
    }
    
    switch(ins_choice) {
        case 1: strcpy(ins_company, "삼성화재"); break;
        case 2: strcpy(ins_company, "한화손해보험"); break;
        case 3: strcpy(ins_company, "KB손해보험"); break;
        case 4: strcpy(ins_company, "DB손해보험"); break;
    }

    sprintf(buffer, "new/%s/%s/%s/%s/%s", id, pw, car_number, car_type, ins_company);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");

    int strLen = recv(sock, buffer, BUF_SIZE, 0);
    if (strLen == SOCKET_ERROR)
        ErrorHandling("recv() error");
    buffer[strLen] = '\0';
    
    if (strcmp(buffer, "1\n") == 0) {
        printf("회원가입이 완료되었습니다.\n");
        return 1;
    } else {
        printf("회원가입에 실패했습니다: %s", buffer);
        return 0;
    }
}

void update_user(SOCKET sock, const char* user_id) {
    char buffer[BUF_SIZE];
    char update_type[2];
    char new_value[256];
    int choice;

    while (1) {
        printf("\n=== 사용자 정보 수정 ===\n");
        printf("1. 비밀번호 변경\n");
        printf("2. 차번호 수정\n");
        printf("3. 차종류 수정(SUV, 승용차, 대형차)\n");
        printf("4. 보험사 수정\n");
        printf("5. 전체 정보 수정\n");
        printf("6. 이전 메뉴로\n");
        printf("선택: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 6) return;

        switch (choice) {
            case 1: 
                printf("새 비밀번호: ");
                get_password_securely(new_value, sizeof(new_value));
                sprintf(update_type, "1");
                break;
            case 2:
                printf("새 차번호: ");
                if (!fgets(new_value, sizeof(new_value), stdin))
                    ErrorHandling("입력 오류");
                new_value[strcspn(new_value, "\r\n")] = '\0';
                sprintf(update_type, "2");
                break;
            case 3: 
                printf("\n=== 차종류 선택 ===\n");
                printf("1. SUV\n");
                printf("2. 승용차\n");
                printf("3. 대형차\n");
                printf("4. 이전 메뉴로\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar(); 

                if (choice == 4) continue;

                switch (choice) {
                    case 1: strcpy(new_value, "SUV"); break;
                    case 2: strcpy(new_value, "승용차"); break;
                    case 3: strcpy(new_value, "대형차"); break;
                    default: 
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                sprintf(update_type, "3");
                break;

            case 4: 
                printf("\n=== 보험사 선택 ===\n");
                printf("1. 삼성화재\n");
                printf("2. 한화손해보험\n");
                printf("3. KB손해보험\n");
                printf("4. DB손해보험\n");
                printf("5. 이전 메뉴로\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar();
                if (choice == 5) continue;
                switch (choice) {
                    case 1: strcpy(new_value, "삼성화재"); break;
                    case 2: strcpy(new_value, "한화손해보험"); break;
                    case 3: strcpy(new_value, "KB손해보험"); break;
                    case 4: strcpy(new_value, "DB손해보험"); break;
                    default: 
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                sprintf(update_type, "4");
                break;
            case 5:
                printf("새 비밀번호: ");
                get_password_securely(new_value, sizeof(new_value));
                strcat(new_value, "/");

                printf("새 차번호: ");
                char temp[256];
                if (!fgets(temp, sizeof(temp), stdin))
                    ErrorHandling("입력 오류");
                temp[strcspn(temp, "\r\n")] = '\0';
                strcat(new_value, temp);
                strcat(new_value, "/");

                printf("\n=== 차종류 선택 ===\n");
                printf("1. SUV\n");
                printf("2. 승용차\n");
                printf("3. 대형차\n");
                printf("4. 이전 메뉴로\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar();

                if (choice == 4) continue; 

                switch (choice) {
                    case 1: strcat(new_value, "SUV"); break;
                    case 2: strcat(new_value, "승용차"); break;
                    case 3: strcat(new_value, "대형차"); break;
                    default: 
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                strcat(new_value, "/");

                printf("\n=== 보험사 선택 ===\n");
                printf("1. 삼성화재\n");
                printf("2. 한화손해보험\n");
                printf("3. KB손해보험\n");
                printf("4. DB손해보험\n");
                printf("5. 이전 메뉴로\n");
                printf("선택: ");
                scanf("%d", &choice);
                getchar(); 

                if (choice == 5) continue; 

                switch (choice) {
                    case 1: strcat(new_value, "삼성화재"); break;
                    case 2: strcat(new_value, "한화손해보험"); break;
                    case 3: strcat(new_value, "KB손해보험"); break;
                    case 4: strcat(new_value, "DB손해보험"); break;
                    default: 
                        printf("잘못된 선택입니다.\n");
                        continue;
                }
                sprintf(update_type, "5");
                break;

            default:
                printf("잘못된 선택입니다.\n");
                continue;
        }
        sprintf(buffer, "UF/%s/%s/%s", update_type, user_id, new_value);
        if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
            ErrorHandling("send() error");

        int strLen = recv(sock, buffer, BUF_SIZE, 0);
        if (strLen == SOCKET_ERROR)
            ErrorHandling("recv() error");
        buffer[strLen] = '\0';
        
        printf("%s", buffer);
    }
}

void show_accident_guide(SOCKET sock) {
    char buffer[BUF_SIZE];

    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, "AC");
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");

    int strLen;
    
    while ((strLen = recv(sock, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[strLen] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "=== 전송 완료 ===") != NULL) {
            break;
        }
    }
    
    if (strLen == SOCKET_ERROR) {
        ErrorHandling("recv() error");
    }
}

void show_law_guide(SOCKET sock) {
    char buffer[BUF_SIZE];

    if (send(sock, "LAW", 3, 0) == SOCKET_ERROR)
        ErrorHandling("send() error");

    int strLen;
    
    while ((strLen = recv(sock, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[strLen] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "=== 전송 완료 ===") != NULL) {
            break;
        }
    }
    
    if (strLen == SOCKET_ERROR) {
        ErrorHandling("recv() error");
    }
}

void show_insurance_guide(SOCKET sock, const char* user_id) {
    char buffer[BUF_SIZE];
    int guide_number; 
    
    printf("\n=== 보험 안내 번호 선택 ===\n");
    printf("1. 보험 안내\n");
    printf("2. 보험 절차\n");
    printf("3. 보험 항목\n");
    printf("4. 이전 메뉴로\n");
    printf("선택: ");
    scanf("%d", &guide_number);
    getchar(); 
    if (guide_number == 4) return;
    if (guide_number < 1 || guide_number > 3) {
        printf("잘못된 선택입니다.\n");
        return;
    }
    
    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, "BS/%s/%d\n", user_id, guide_number);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");
    
    int strLen;
    
    while ((strLen = recv(sock, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[strLen] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "=== 전송 완료 ===") != NULL) {
            break;
        }
    }
    
    if (strLen == SOCKET_ERROR) {
        ErrorHandling("recv() error");
    }
}

void show_compensation_guide(SOCKET sock, const char* user_id) {
    char buffer[BUF_SIZE];
    int guide_number; 
    
    printf("\n=== 보상 안내 번호 선택 ===\n");
    printf("1. 대인 배상\n");
    printf("2. 대물 배상\n");
    printf("3. 자기신체사고\n");
    printf("4. 자동차상해\n");
    printf("5. 자기차량손해\n");
    printf("6. 이전 메뉴로\n");
    printf("선택: ");
    scanf("%d", &guide_number);
    getchar(); 
    
    if (guide_number == 6) return; 
    
    if (guide_number < 1 || guide_number > 5) {
        printf("잘못된 선택입니다.\n");
        return;
    }
    
    
    memset(buffer, 0, sizeof(buffer));
    

    sprintf(buffer, "BI/%s/%d", user_id, guide_number);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");
    int strLen;
    
    while ((strLen = recv(sock, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[strLen] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "=== 전송 완료 ===") != NULL) {
            break;
        }
    }
    
    if (strLen == SOCKET_ERROR) {
        ErrorHandling("recv() error");
    }
}

void ACCIDENT_DATA(SOCKET sock, const char* user_id) {
    char buffer[BUF_SIZE];
    char when[20], what[100], how[200];
    int role_choice;
    
    printf("\n=== 사고 정보 입력 ===\n");
    printf("(입력 중 '이전'을 입력하면 이전 단계로 돌아갑니다)\n");

    while (1) {
        printf("언제 발생했나요? (예: 2024-04-29): ");
        if (!fgets(when, sizeof(when), stdin))
            ErrorHandling("입력 오류");
        when[strcspn(when, "\r\n")] = '\0';

        if (strcmp(when, "취소") == 0 || strcmp(when, "cancel") == 0) {
            printf("사고 정보 입력을 취소합니다.\n");
            return;
        }

        if (strcmp(when, "이전") == 0 || strcmp(when, "back") == 0) {
            printf("사고 정보 입력을 취소합니다.\n");
            return;
        }
        
        if (strlen(when) > 0) {
            break;
        } else {
            printf("날짜를 입력해주세요.\n");
        }
    }

    while (1) {
        printf("무슨 사고가 발생했나요?: ");
        if (!fgets(what, sizeof(what), stdin))
            ErrorHandling("입력 오류");
        what[strcspn(what, "\r\n")] = '\0';

        if (strcmp(what, "취소") == 0 || strcmp(what, "cancel") == 0) {
            printf("사고 정보 입력을 취소합니다.\n");
            return;
        }
        
        if (strcmp(what, "이전") == 0 || strcmp(what, "back") == 0) {
            printf("이전 단계로 돌아갑니다.\n");
            continue; 
        }
        
        if (strlen(what) > 0) {
            break;
        } else {
            printf("사고 내용을 입력해주세요.\n");
        }
    }
    
    while (1) {
        printf("어떻게 발생했나요?: ");
        if (!fgets(how, sizeof(how), stdin))
            ErrorHandling("입력 오류");
        how[strcspn(how, "\r\n")] = '\0';
        

        if (strcmp(how, "취소") == 0 || strcmp(how, "cancel") == 0) {
            printf("사고 정보 입력을 취소합니다.\n");
            return;
        }

        if (strcmp(how, "이전") == 0 || strcmp(how, "back") == 0) {
            printf("이전 단계로 돌아갑니다.\n");
            continue; 
        }

        if (strlen(how) > 0) {
            break;
        } else {
            printf("사고 발생 과정을 입력해주세요.\n");
        }
    }

    while (1) {
        printf("\n역할을 선택하세요:\n");
        printf("1. 가해자\n");
        printf("2. 피해자\n");
        printf("3. 이전 단계로\n");
        printf("4. 취소\n");
        printf("선택: ");
        scanf("%d", &role_choice);
        getchar(); 
        if (role_choice == 3) {
            printf("이전 단계로 돌아갑니다.\n");
            continue; 
        }
        
        if (role_choice == 4) {
            printf("사고 정보 입력을 취소합니다.\n");
            return;
        }
        
        if (role_choice < 1 || role_choice > 2) {
            printf("잘못된 선택입니다. 다시 선택해주세요.\n");
            continue;
        }
        
        break;
    }
    
    const char* role = (role_choice == 1) ? "가해자" : "피해자";

    printf("\n=== 입력된 사고 정보 확인 ===\n");
    printf("발생 시기: %s\n", when);
    printf("사고 내용: %s\n", what);
    printf("발생 과정: %s\n", how);
    printf("역할: %s\n", role);
    
    printf("\n위 정보가 맞습니까? (y/n): ");
    char confirm[10];
    if (!fgets(confirm, sizeof(confirm), stdin))
        ErrorHandling("입력 오류");
    confirm[strcspn(confirm, "\r\n")] = '\0';
    
    if (strcmp(confirm, "n") == 0 || strcmp(confirm, "N") == 0 || 
        strcmp(confirm, "no") == 0 || strcmp(confirm, "No") == 0) {
        printf("사고 정보 입력을 다시 시작합니다.\n");
        return;
    }

    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, "AI/%s/%s/%s/%s/%s", user_id, when, what, how, role);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        ErrorHandling("send() error");

    memset(buffer, 0, sizeof(buffer));

    int strLen = recv(sock, buffer, BUF_SIZE, 0);
    if (strLen == SOCKET_ERROR)
        ErrorHandling("recv() error");
    buffer[strLen] = '\0';
    
    printf("%s", buffer);
}

void show_main_menu(SOCKET sock, const char* user_id) {
    while (1) {
        system("cls"); // Clear console screen
        printf("\n=== 프로그램 기능 안내 ===\n");
        printf("1. 사용자 정보 수정 기능\n");
        printf("2. 사고 정보 입력 기능\n");
        printf("3. 보험사별 사고 유형 안내\n");
        printf("4. 보험사별 보상 안내\n");
        printf("5. 사고대처 방법 안내\n");
        printf("6. 12대 중과실 법률 안내\n");
        printf("7. 프로그램 종료\n");
        printf("원하는 메뉴(번호)를 선택하세요: ");
        
        char choice_str[10]; 
        if (!fgets(choice_str, sizeof(choice_str), stdin)) {
            ErrorHandling("메뉴 입력 오류");
        }
        choice_str[strcspn(choice_str, "\r\n")] = '\0'; 
        
 
        if (strcmp(choice_str, "1") == 0) { 
            update_user(sock, user_id);
            printf("\n계속하려면 엔터를 누르세요...");
            getchar();
        } else if (strcmp(choice_str, "2") == 0) { 
            ACCIDENT_DATA(sock, user_id);
            printf("\n계속하려면 엔터를 누르세요...");
            getchar();
        } else if (strcmp(choice_str, "3") == 0) { 
            show_compensation_guide(sock, user_id);
            printf("\n계속하려면 엔터를 누르세요...");
            getchar();
        } else if (strcmp(choice_str, "4") == 0) { 
            show_insurance_guide(sock, user_id);
            printf("\n계속하려면 엔터를 누르세요...");
            getchar();
        } else if (strcmp(choice_str, "5") == 0) { 
            show_accident_guide(sock);
            printf("\n계속하려면 엔터를 누르세요...");
            getchar();
        } else if (strcmp(choice_str, "6") == 0) { 
            show_law_guide(sock);
            printf("\n계속하려면 엔터를 누르세요...");
            getchar();
        } else if (strcmp(choice_str, "7") == 0) { 
            if (send(sock, "EX", 2, 0) == SOCKET_ERROR)
                ErrorHandling("send() error");
            closesocket(sock);
            WSACleanup();
            exit(0);
        } else {
            printf("\n잘못된 선택입니다. 다시 선택해주세요.\n");
        }
    }
}

int main(void) {
    SetConsoleOutputCP(CP_UTF8); 
    SetConsoleCP(CP_UTF8);
    
    
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in servAddr;
    char buffer[BUF_SIZE];
    char server_ip_str[16]; 
    int strLen;

    char current_user_id[20] = {0};

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error");

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        ErrorHandling("socket() error");

    printf("서버 IP 주소를 입력하세요: ");
    if (!fgets(server_ip_str, sizeof(server_ip_str), stdin))
        ErrorHandling("IP 입력 오류");
    server_ip_str[strcspn(server_ip_str, "\r\n")] = '\0'; 

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(server_ip_str); 
    servAddr.sin_port        = htons(SERVER_PORT);


    if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        ErrorHandling("connect() error");

    fputs("서버에 연결되었습니다...\n\n", stdout);
    
 
    while (1) {
        printf("\n=== 교통사고 처리 안내 시스템 ===\n");
        printf("1. 로그인\n");
        printf("2. 회원가입\n");
        printf("3. 종료\n");
        printf("선택: ");
        
        char choice_str[10]; 
        if (!fgets(choice_str, sizeof(choice_str), stdin)) {
            ErrorHandling("메뉴 입력 오류");
        }
        choice_str[strcspn(choice_str, "\r\n")] = '\0'; 
        
        if (strcmp(choice_str, "1") == 0) { 
            if (login(sock, current_user_id)) {
                show_main_menu(sock, current_user_id); 
            }
        } else if (strcmp(choice_str, "2") == 0) { 
            if (new_user(sock)) {
                printf("\n회원가입이 완료되었습니다. 로그인을 진행해주세요.\n");
                
                if (login(sock, current_user_id)) {
                    show_main_menu(sock, current_user_id); 
                }
            }
        } else if (strcmp(choice_str, "3") == 0) { 
            printf("\n프로그램을 종료합니다.\n");
            closesocket(sock);
            WSACleanup();
            return 0;
        } else {
            printf("\n잘못된 선택입니다. 다시 선택해주세요.\n");
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}