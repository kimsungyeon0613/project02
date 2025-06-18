#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <ctype.h>
#include <errno.h> // for strerror
#include <winsock2.h>
#include <ws2tcpip.h>
#include <locale.h>
#pragma comment(lib, "ws2_32.lib") 
char* strtok_r(char* str, const char* delim, char** saveptr) {
    char* result;
    if (str == NULL) {
        str = *saveptr;
    }
    if (str == NULL) {
        return NULL;
    }

    // Skip leading delimiters
    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    result = str;
    // Find the next delimiter or end of string
    str += strcspn(str, delim);

    if (*str != '\0') {
        *str = '\0';
        *saveptr = str + 1;
    } else {
        *saveptr = NULL;
    }
    return result;
}

#define PORT_NUM 5050 // 서버 포트 번호
#define BUF_SIZE 1024
#define MAX_USERS 1000
#define MAX_ACCIDENTS 1000
#define MAX_INFO_LENGTH 30
#define MAX_CLNT 100 // MAX_CLNT 정의 추가
#define USER_FILE "users02.txt" // 사용자 정보 파일 경로
#define ACCIDENT_FILE "C:\\Coding\\project\\project02\\accident.txt"// 사고 정보 입력 파일 경로
#define SAMSUNG_ACCIDENT_FILE "C:\\Coding\\project\\project02\\samsung_AC.txt"// 삼성화재 사고 정보 파일 경로
#define SAMSUNG_CPS_FILE "C:\\Coding\\project\\project02\\samsung_CPS.txt" // 삼성화재 보험안내 정보 파일 경로
#define HANWHA_ACCIDENT_FILE "C:\\Coding\\project\\project02\\hanhwa_AC.txt"    // 한화손해보험 사고 정보 파일 경로
#define HANWHA_CPS_FILE "C:\\Coding\\project\\project02\\hanhwa_CPS.txt" // 한화손해보험 보험안내 정보 파일 경로
#define KB_ACCIDENT_FILE "C:\\Coding\\project\\project02\\kb_AC.txt" // KB손해보험 사고 정보 파일 경로
#define KB_CPS_FILE "C:\\Coding\\project\\project02\\kb_CPS.txt" // KB손해보험 보험안내 정보 파일 경로
#define DB_ACCIDENT_FILE "C:\\Coding\\project\\project02\\db_AC.txt" // DB손해보험 사고 정보 파일 경로
#define DB_CPS_FILE "C:\\Coding\\project\\project02\\db_CPS.txt" // DB손해보험 보험안내 정보 파일 경로
#define ACCIDENT_LAW_FILE "C:\\Coding\\project\\project02\\accident_law.txt" // 사고 법률 안내 파일 경로
#define ACCIDENT_GUIDE_FILE "C:\\Coding\\project\\project02\\accident_guide.txt"


void trim_trailing_newlines(char* str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

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

// 사용자 정보 캐시 구조체
typedef struct {
    char user_id[20];
    char insurance_company[20];
    time_t last_updated;
} USER_CACHE;

USER_CACHE user_cache[MAX_USERS];
int cache_count = 0;

//사용자 보험사 정보 찾기
int find_user_in_cache(const char* user_id, char* insurance_company_buf, size_t buf_size) {
    for (int i = 0; i < cache_count; i++) {
        if (strcmp(user_cache[i].user_id, user_id) == 0) {
            strncpy(insurance_company_buf, user_cache[i].insurance_company, buf_size - 1);
            insurance_company_buf[buf_size - 1] = '\0';
            printf("find_user_in_cache - 캐시에서 찾음: %s -> %s\n", user_id, insurance_company_buf);
            return 1;
        }
    }
    return 0;
}

//사용자 정보 추가/업데이트
void update_user_cache(const char* user_id, const char* insurance_company) {
    for (int i = 0; i < cache_count; i++) {
        if (strcmp(user_cache[i].user_id, user_id) == 0) {
            strncpy(user_cache[i].insurance_company, insurance_company, sizeof(user_cache[i].insurance_company) - 1);
            user_cache[i].insurance_company[sizeof(user_cache[i].insurance_company) - 1] = '\0';
            user_cache[i].last_updated = time(NULL);
            printf("update_user_cache - 캐시 업데이트: %s -> %s\n", user_id, insurance_company);
            return;
        }
    }

    if (cache_count < MAX_USERS) {
        strncpy(user_cache[cache_count].user_id, user_id, sizeof(user_cache[cache_count].user_id) - 1);
        user_cache[cache_count].user_id[sizeof(user_cache[cache_count].user_id) - 1] = '\0';
        strncpy(user_cache[cache_count].insurance_company, insurance_company, sizeof(user_cache[cache_count].insurance_company) - 1);
        user_cache[cache_count].insurance_company[sizeof(user_cache[cache_count].insurance_company) - 1] = '\0';
        user_cache[cache_count].last_updated = time(NULL);
        cache_count++;
        printf("update_user_cache - 새 캐시 추가: %s -> %s\n", user_id, insurance_company);
    }
}

// 로그인 인증
int UserOk(const char* id, const char* password) {
    printf("DEBUG: UserOk 함수 진입 - ID: %s\n", id);
    
    FILE* file = fopen(USER_FILE, "r");
    if (!file) {
        printf("DEBUG: UserOk: 파일 열기 실패\n");
        return 0;
    }

    char line[256];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; 
        
        char *saveptr;
        char *current_id = strtok_r(line, "/", &saveptr);
        if (!current_id) continue;

        if (strcmp(current_id, id) == 0) {
            char *current_pw = strtok_r(NULL, "/", &saveptr);
            if (current_pw && strcmp(current_pw, password) == 0) {
                found = 1;
                break;
            }
        }
    }

    fclose(file);
    printf("DEBUG: UserOk: 로그인 %s\n", found ? "성공" : "실패");
    return found;
}

// 회원가입
int handle_new_user(const char* id, const char* password, const char* car_number, 
                   const char* car_type, const char* insurance) {
    printf("handle_new_user 함수 진입\n");
    printf("입력된 정보 - ID: %s, PW: %s, 차량번호: %s, 차종류: %s, 보험사: %s\n", 
           id, password, car_number, car_type, insurance);
    printf("USER_FILE 경로: %s\n", USER_FILE);

    if (!id || strlen(id) == 0) {
        printf("ID가 비어있음\n");
        return 0;
    }
    if (!password || strlen(password) == 0) {
        printf("비밀번호가 비어있음\n");
        return 0;
    }
    if (!car_number || strlen(car_number) == 0) {
        printf("차량번호가 비어있음\n");
        return 0;
    }
    if (!car_type || strlen(car_type) == 0) {
        printf("차종류가 비어있음\n");
        return 0;
    }
    if (!insurance || strlen(insurance) == 0) {
        printf("보험사가 비어있음\n");
        return 0;
    }

    // ID 중복 체크
    printf("USER_FILE 열기 시도 - %s\n", USER_FILE);
    FILE *file = fopen(USER_FILE, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\r\n")] = '\0';  
            printf("파일에서 읽은 라인: %s\n", line);
            
            if (strcmp(line, "ID/PW/차번호/차종류/보험사") == 0) {
                continue;
            }
            
            char *saveptr;
            char *current_id = strtok_r(line, "/", &saveptr);
            if (current_id && strcmp(current_id, id) == 0) {
                printf("ID 중복 - %s\n", id);
                fclose(file);
                return 0; 
            }
        }
        fclose(file);
    }
    file = fopen(USER_FILE, "a");
    if (!file) {
        printf(" 파일 열기 실패 - %s\n", strerror(errno));
        printf("파일 경로: %s\n", USER_FILE);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    
    if (file_size == 0) {
        fprintf(file, "ID/PW/차번호/차종류/보험사\n");
        printf("handle_new_user: 헤더 추가\n");
    }

    char clean_id[20], clean_password[20], clean_car_number[20], clean_car_type[20], clean_insurance[20];
    strncpy(clean_id, id, sizeof(clean_id) - 1);
    clean_id[sizeof(clean_id) - 1] = '\0';
    clean_id[strcspn(clean_id, "\r\n")] = '\0';
    
    strncpy(clean_password, password, sizeof(clean_password) - 1);
    clean_password[sizeof(clean_password) - 1] = '\0';
    clean_password[strcspn(clean_password, "\r\n")] = '\0';
    
    strncpy(clean_car_number, car_number, sizeof(clean_car_number) - 1);
    clean_car_number[sizeof(clean_car_number) - 1] = '\0';
    clean_car_number[strcspn(clean_car_number, "\r\n")] = '\0';
    
    strncpy(clean_car_type, car_type, sizeof(clean_car_type) - 1);
    clean_car_type[sizeof(clean_car_type) - 1] = '\0';
    clean_car_type[strcspn(clean_car_type, "\r\n")] = '\0';
    
    strncpy(clean_insurance, insurance, sizeof(clean_insurance) - 1);
    clean_insurance[sizeof(clean_insurance) - 1] = '\0';
    clean_insurance[strcspn(clean_insurance, "\r\n")] = '\0';
   
    int write_result = fprintf(file, "%s/%s/%s/%s/%s\n", clean_id, clean_password, clean_car_number, clean_car_type, clean_insurance);
    if (write_result < 0) {
        printf("handle_new_user: 파일 쓰기 실패 - %s\n", strerror(errno));
        fclose(file);
        return 0;
    }

    fflush(file);
    fclose(file);

    printf("handle_new_user: 사용자 등록 완료 - ID: %s\n", clean_id);
    printf("handle_new_user: 저장된 내용: %s/%s/%s/%s/%s\n", clean_id, clean_password, clean_car_number, clean_car_type, clean_insurance);
    return 1; 
}

int save_accident_data(const char* id, const char* when, const char* what, 
                 const char* how, const char* role) {

    FILE *f = fopen(ACCIDENT_FILE, "a");
    if (f == NULL) {
        printf("사고 정보 파일 열기 실패: %s\n", strerror(errno));
        return 0;
    }

    fprintf(f, "%s/%s/%s/%s/%s\n", id, when, what, how, role);
    
    fclose(f);
    return 1;
}

int handle_ai_command(const char* message) {
    char msg_copy[1024];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';

    char *token = strtok(msg_copy, "/");
    if (!token || strcmp(token, "AI") != 0) {
        printf("잘못된 명령어 형식입니다.\n");
        return 0;
    }

    char *id = strtok(NULL, "/");
    char *when = strtok(NULL, "/");
    char *what = strtok(NULL, "/");
    char *how = strtok(NULL, "/");
    char *role = strtok(NULL, "/");


    if (!id || !when || !what || !how || !role) {
        printf("필수 필드가 누락되었습니다.\n");
        return 0;
    }


    if (strcmp(role, "가해자") != 0 && strcmp(role, "피해자") != 0) {
        printf("역할은 '가해자' 또는 '피해자'여야 합니다.\n");
        return 0;
    }


    if (save_accident_data(id, when, what, how, role)) {
        printf("사고 정보가 성공적으로 저장되었습니다.\n");
        return 1;
    } else {
        printf("사고 정보 저장에 실패했습니다.\n");
        return 0;
    }
}

// 사용자 정보 수정
int USER_DATAUPDATE(const char* message, char* user_id) {
    printf("USER_DATAUPDATE 함수 진입\n");
    if (!message || !user_id) {
        printf("USER_DATAUPDATE: message 또는 user_id가 NULL\n");
        return 0;
    }

    char msg_copy[1024];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';
    
    char *token = strtok(msg_copy, "/");
    if (!token || strcmp(token, "UF") != 0) {
        printf("USER_DATAUPDATE: 메시지 형식이 UF가 아님\n");
        return 0;
    }

    char *field_str = strtok(NULL, "/");
    if (!field_str) {
        printf("USER_DATAUPDATE: 필드 번호가 없음\n");
        return 0;
    }
    int field = atoi(field_str);

    char *client_msg_user_id = strtok(NULL, "/");
    if (!client_msg_user_id || strcmp(client_msg_user_id, user_id) != 0) {
        printf("불일치 또는 없음\n");
        return 0;
    }

    char *new_value_str = strtok(NULL, "");
    if (!new_value_str) {
        printf("새로운 값이 없음\n");
        return 0;
    }

    // 새 값에서 줄바꿈 문자 제거
    new_value_str[strcspn(new_value_str, "\r\n")] = 0;

    // 기존 사용자 정보 읽기
    FILE *original_file = fopen(USER_FILE, "r");
    if (!original_file) {
        printf("원본 파일 열기 실패\n");
        return 0;
    }

    // 임시 파일 생성
    FILE *temp_file = fopen("temp_users.txt", "w");
    if (!temp_file) {
        printf("임시 파일 생성 실패\n");
        fclose(original_file);
        return 0;
    }

    char line[256];
    int user_found = 0;
    int line_count = 0;

    if (fgets(line, sizeof(line), original_file)) {
        line[strcspn(line, "\r\n")] = '\0';
        fprintf(temp_file, "%s\n", line);
        line_count++;
    }

    while (fgets(line, sizeof(line), original_file)) {
        line[strcspn(line, "\r\n")] = '\0';
        line_count++;

        char file_id[20] = {0};
        char file_pw[20] = {0};
        char file_car_num[20] = {0};
        char file_car_type[20] = {0};
        char file_insurance[20] = {0};

        if (sscanf(line, "%19[^/]/%19[^/]/%19[^/]/%19[^/]/%19[^\n]",
                   file_id, file_pw, file_car_num, file_car_type, file_insurance) == 5) {
            
            if (strcmp(file_id, user_id) == 0) {

                user_found = 1;
                printf("USER_DATAUPDATE: 사용자 %s 정보 수정 중...\n", user_id);
                
    
                switch (field) {
                    case 1:
                        fprintf(temp_file, "%s/%s/%s/%s/%s\n", 
                                file_id, new_value_str, file_car_num, file_car_type, file_insurance);
                        printf("USER_DATAUPDATE: 비밀번호를 %s로 수정\n", new_value_str);
                        break;
                    case 2: 
                        fprintf(temp_file, "%s/%s/%s/%s/%s\n", 
                                file_id, file_pw, new_value_str, file_car_type, file_insurance);
                        printf("USER_DATAUPDATE: 차량번호를 %s로 수정\n", new_value_str);
                        break;
                    case 3:
                        fprintf(temp_file, "%s/%s/%s/%s/%s\n", 
                                file_id, file_pw, file_car_num, new_value_str, file_insurance);
                        printf("USER_DATAUPDATE: 차종류를 %s로 수정\n", new_value_str);
                        break;
                    case 4:
                        fprintf(temp_file, "%s/%s/%s/%s/%s\n", 
                                file_id, file_pw, file_car_num, file_car_type, new_value_str);
                        printf("USER_DATAUPDATE: 보험사를 %s로 수정\n", new_value_str);
                        update_user_cache(file_id, new_value_str);
                        break;
                    case 5:
                        {
                            char *temp_new_value = strdup(new_value_str);
                            char *pw_part = strtok(temp_new_value, "/");
                            char *car_num_part = strtok(NULL, "/");
                            char *car_type_part = strtok(NULL, "/");
                            char *insurance_part = strtok(NULL, "/");
                            
                            if (pw_part && car_num_part && car_type_part && insurance_part) {
                                fprintf(temp_file, "%s/%s/%s/%s/%s\n", 
                                        file_id, pw_part, car_num_part, car_type_part, insurance_part);
                                printf("USER_DATAUPDATE: 전체 정보 수정 완료\n");
                        
                                update_user_cache(file_id, insurance_part);
                            } else {
                                printf("USER_DATAUPDATE: 전체 정보 형식 오류\n");
                                fprintf(temp_file, "%s\n", line); 
                            }
                            free(temp_new_value);
                        }
                        break;
                    default:
                        printf("USER_DATAUPDATE: 잘못된 필드 번호: %d\n", field);
                        fprintf(temp_file, "%s\n", line); 
                        break;
                }
            } else {
               
                fprintf(temp_file, "%s\n", line);
            }
        } else {
         
            fprintf(temp_file, "%s\n", line);
        }
    }

    fclose(original_file);
    fclose(temp_file);

    if (!user_found) {
        printf("USER_DATAUPDATE: 사용자 %s를 찾을 수 없음\n", user_id);
        remove("temp_users.txt");
        return 0;
    }

    if (remove(USER_FILE) != 0) {
        printf("USER_DATAUPDATE: 원본 파일 삭제 실패\n");
        remove("temp_users.txt");
        return 0;
    }

    if (rename("temp_users.txt", USER_FILE) != 0) {
        printf("USER_DATAUPDATE: 임시 파일을 원본 파일로 교체 실패\n");
        return 0;
    }
    FILE* sync_file = fopen(USER_FILE, "r");
    if (sync_file) {
        fclose(sync_file);
        printf("USER_DATAUPDATE: 파일 동기화 완료\n");
    }

    printf("USER_DATAUPDATE: 사용자 %s 정보 업데이트 완료\n", user_id);
    return 1;
}

void show_accident_guide(SOCKET sock) {
    printf("show_accident_guide 함수 진입\n");
    FILE* file = fopen(ACCIDENT_GUIDE_FILE, "rb");
    if (!file) {
        printf("show_accident_guide: 사고 대처 안내 파일 열기 실패: %s\n", strerror(errno));
        send(sock, "사고 대처 안내 정보를 불러올 수 없습니다.\n",
             strlen("사고 대처 안내 정보를 불러올 수 없습니다.\n"), 0);
        return;
    }
    printf("show_accident_guide: 사고 대처 안내 파일 열림: %s\n", ACCIDENT_GUIDE_FILE);

    char header[] = "=== 사고 발생 시 대처방법 안내 ===\n\n";
    send(sock, header, strlen(header), 0);

    char buffer[1024];
    int total_sent = 0;
    int chunk_count = 0;
    
    while (fgets(buffer, sizeof(buffer), file)) {
        trim_trailing_newlines(buffer);
        strcat(buffer, "\n");
        
        int sent = send(sock, buffer, strlen(buffer), 0);
        if (sent == SOCKET_ERROR) {
            printf("show_accident_guide: 전송 오류 발생\n");
            break;
        }
        total_sent += sent;
        chunk_count++;
        
    }
    
    fclose(file);
    
    char end_marker[] = "\n=== 전송 완료 ===\n";
    send(sock, end_marker, strlen(end_marker), 0);
    
    printf("show_accident_guide: 총 %d 청크, %d 바이트 전송 완료\n", chunk_count, total_sent);
    printf("show_accident_guide: 함수 종료\n");
}

void show_law_guide(SOCKET sock) {
    printf("DEBUG: show_law_guide 함수 진입\n");
    FILE* file = fopen(ACCIDENT_LAW_FILE, "rb");
    if (!file) {
        printf("show_law_guide: 12대 중과실 법률 안내 파일 열기 실패: %s\n", strerror(errno));
        send(sock, "12대 중과실 법률 안내를 불러올 수 없습니다.\n", 
             strlen("12대 중과실 법률 안내를 불러올 수 없습니다.\n"), 0);
        return;
    }
    printf("show_law_guide: 12대 중과실 법률 안내 파일 열림: %s\n", ACCIDENT_LAW_FILE);

    char header[] = "=== 12대 중과실 법률 안내 ===\n\n";
    send(sock, header, strlen(header), 0);

    char buffer[1024];
    int total_sent = 0;
    int chunk_count = 0;
    
    while (fgets(buffer, sizeof(buffer), file)) {
        trim_trailing_newlines(buffer); 
        strcat(buffer, "\n"); 
        
        int sent = send(sock, buffer, strlen(buffer), 0);
        if (sent == SOCKET_ERROR) {
            printf("show_law_guide: 전송 오류 발생\n");
            break;
        }
        total_sent += sent;
        chunk_count++;

        Sleep(10);
    }
    
    fclose(file);

    char end_marker[] = "\n=== 전송 완료 ===\n";
    send(sock, end_marker, strlen(end_marker), 0);
    
    printf("show_law_guide: 총 %d 청크, %d 바이트 전송 완료\n", chunk_count, total_sent);
    printf("show_law_guide: 함수 종료\n");
}

int get_user_insurance_company(const char* user_id, char* insurance_company_buf, size_t buf_size) {
    printf("get_user_insurance_company - 사용자 ID: %s\n", user_id);
    printf("get_user_insurance_company - USER_FILE 경로: %s\n", USER_FILE);

    if (find_user_in_cache(user_id, insurance_company_buf, buf_size)) {
        return 1;
    }

    Sleep(50);
    

    FILE* check_file = fopen(USER_FILE, "r");
    if (!check_file) {
        printf("get_user_insurance_company - 파일 존재하지 않음: %s\n", strerror(errno));
        return 0;
    }
    fclose(check_file);
    
    FILE* file = fopen(USER_FILE, "r");
    if (!file) {
        printf("get_user_insurance_company - 파일 열기 실패: %s\n", strerror(errno));
        return 0;
    }
    setvbuf(file, NULL, _IONBF, 0);

    char line[256];
    int line_count = 0;
    int found_user = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_count++;
        line[strcspn(line, "\r\n")] = '\0';
        printf("get_user_insurance_company - 라인 %d: %s\n", line_count, line);
        if (strcmp(line, "ID/PW/차번호/차종류/보험사") == 0) {
            printf("get_user_insurance_company - 헤더 라인 건너뛰기\n");
            continue;
        }

        char file_id[20] = {0};
        char file_pw[20] = {0};
        char file_car_num[20] = {0};
        char file_car_type[20] = {0};
        char file_insurance[20] = {0};

        if (sscanf(line, "%19[^/]/%19[^/]/%19[^/]/%19[^/]/%19[^\n]",
                   file_id, file_pw, file_car_num, file_car_type, file_insurance) == 5) {
            printf("get_user_insurance_company - 파싱된 정보: ID=%s, 보험사=%s\n", 
                   file_id, file_insurance);
            
            if (strcmp(user_id, file_id) == 0) {
                found_user = 1;
                strncpy(insurance_company_buf, file_insurance, buf_size - 1);
                insurance_company_buf[buf_size - 1] = '\0';
                update_user_cache(user_id, file_insurance);
                
                printf("get_user_insurance_company - 보험사 정보 추출 성공: %s\n", 
                       insurance_company_buf);
                fclose(file);
                return 1;
            }
        } else {
            printf("라인 %d 파싱 실패: %s\n", line_count, line);
        }
    }

    if (!found_user) {
        printf("사용자 %s를 찾을 수 없음\n", user_id);
    } else {
        printf("보험사 정보를 찾을 수 없음\n");
    }
    
    fclose(file);
    return 0;
}

void BI_GUIDE(SOCKET clientSock, const char* msg) {
    printf("클라이언트 요청: %s\n", msg);
    char user_insurance_company[20];
    char buffer[BUF_SIZE];
    int guide_number;

    char msg_copy[BUF_SIZE];
    strncpy(msg_copy, msg, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';

    char *temp_msg = msg_copy;
    char *token = (char*)strtok_r(temp_msg, "/", &temp_msg); 
    if (!token) return;

    char *user_id_from_msg = (char*)strtok_r(NULL, "/", &temp_msg);
    if (!user_id_from_msg) return;

    char *guide_num_str = (char*)strtok_r(NULL, "/", &temp_msg);
    if (!guide_num_str) return;
    guide_number = atoi(guide_num_str);

    if (!get_user_insurance_company(user_id_from_msg, user_insurance_company, sizeof(user_insurance_company))) {
        printf("사용자 %s의 보험사 정보를 가져올 수 없음\n", user_id_from_msg);
        send(clientSock, "사용자 보험사 정보를 가져올 수 없습니다.\n", 
             strlen("사용자 보험사 정보를 가져올 수 없습니다.\n"), 0);
        return;
    }

    printf("사용자 %s의 보험사: %s\n", user_id_from_msg, user_insurance_company);

    char file_path[256];
    if (strcmp(user_insurance_company, "삼성화재") == 0) {
        strcpy(file_path, SAMSUNG_ACCIDENT_FILE);
    } else if (strcmp(user_insurance_company, "한화손해보험") == 0) {
        strcpy(file_path, HANWHA_ACCIDENT_FILE);
    } else if (strcmp(user_insurance_company, "KB손해보험") == 0) {
        strcpy(file_path, KB_ACCIDENT_FILE);
    } else if (strcmp(user_insurance_company, "DB손해보험") == 0) {
        strcpy(file_path, DB_ACCIDENT_FILE);
    } else {
        char error_msg[] = "알 수 없는 보험사입니다.\n";
        send(clientSock, error_msg, strlen(error_msg), 0);
        return;
    }

    FILE* guide_file_ptr = fopen(file_path, "r");
    if (!guide_file_ptr) {
        send(clientSock, "보험 안내 정보를 불러올 수 없습니다.\n", 
             strlen("보험 안내 정보를 불러올 수 없습니다.\n"), 0);
        return;
    }

    char file_content[8192] = {0}; 
    while (fgets(buffer, sizeof(buffer), guide_file_ptr)) {
        strcat(file_content, buffer);
    }
    fclose(guide_file_ptr);

    printf("요청된 섹션 번호: %d\n", guide_number);

  
    char *content_ptr = file_content;
    char *section_start = NULL;
    char *section_end = NULL;
    int found_section = 0;
    
    while (*content_ptr) {
        if (*content_ptr == '+') {
            int plus_count = 0;
            char *temp = content_ptr;
            while (*temp == '+') {
                plus_count++;
                temp++;
            }
            if (plus_count == guide_number) {
                section_start = content_ptr + plus_count; 
                while (*section_start == ' ' || *section_start == '.') section_start++; 
                section_end = strstr(section_start, "//");
                found_section = 1;
                break;
            }
        }
        content_ptr++;
    }

    if (!found_section) {
        send(clientSock, "요청하신 보험 안내 정보를 찾을 수 없습니다.\n", 
             strlen("요청하신 보험 안내 정보를 찾을 수 없습니다.\n"), 0);
        return;
    }

    size_t section_length = section_end ? (section_end - section_start) : strlen(section_start);
    char *current_pos = section_start;
    int total_sent = 0;
    int chunk_count = 0;
    
    while (section_length > 0) {
        size_t chunk_size = (section_length > 1024) ? 1024 : section_length;
        char chunk[1025];
        strncpy(chunk, current_pos, chunk_size);
        chunk[chunk_size] = '\0';
        
        int sent = send(clientSock, chunk, strlen(chunk), 0);
        if (sent == SOCKET_ERROR) {
            printf("전송 오류 발생\n");
            break;
        }
        
        total_sent += sent;
        chunk_count++;
        current_pos += chunk_size;
        section_length -= chunk_size;
        

        Sleep(10);
    }
    
    char end_marker[] = "\n=== 전송 완료 ===\n";
    send(clientSock, end_marker, strlen(end_marker), 0);
    
    printf("총 %d 청크, %d 바이트 전송 완료\n", chunk_count, total_sent);
}

void BS_GUIDE(SOCKET clientSock, const char* msg) {
    printf("클라이언트 요청: %s\n", msg);
    char user_insurance_company[20];
    char buffer[BUF_SIZE];
    int guide_number;

    char msg_copy[BUF_SIZE];
    strncpy(msg_copy, msg, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';

    char *temp_msg = msg_copy;
    char *token = (char*)strtok_r(temp_msg, "/", &temp_msg); // "BS"
    if (!token) return;

    char *user_id_from_msg = (char*)strtok_r(NULL, "/", &temp_msg);
    if (!user_id_from_msg) return;

    char *guide_num_str = (char*)strtok_r(NULL, "/", &temp_msg);
    if (!guide_num_str) return;
    guide_number = atoi(guide_num_str);

    if (!get_user_insurance_company(user_id_from_msg, user_insurance_company, sizeof(user_insurance_company))) {
        printf("사용자 %s의 보험사 정보를 가져올 수 없음\n", user_id_from_msg);
        char error_msg[] = "사용자 보험사 정보를 가져올 수 없습니다.\n";
        send(clientSock, error_msg, strlen(error_msg), 0);
        return;
    }

    printf("사용자 %s의 보험사: %s\n", user_id_from_msg, user_insurance_company);

    char file_path[256];
    if (strcmp(user_insurance_company, "삼성화재") == 0) {
        strcpy(file_path, SAMSUNG_CPS_FILE);
    } else if (strcmp(user_insurance_company, "한화손해보험") == 0) {
        strcpy(file_path, HANWHA_CPS_FILE);
    } else if (strcmp(user_insurance_company, "KB손해보험") == 0) {
        strcpy(file_path, KB_CPS_FILE);
    } else if (strcmp(user_insurance_company, "DB손해보험") == 0) {
        strcpy(file_path, DB_CPS_FILE);
    } else {
        char error_msg[] = "알 수 없는 보험사입니다.\n";
        send(clientSock, error_msg, strlen(error_msg), 0);
        return;
    }

    FILE* guide_file_ptr = fopen(file_path, "r");
    if (!guide_file_ptr) {
        char error_msg[] = "보험 안내 정보를 불러올 수 없습니다.\n";
        send(clientSock, error_msg, strlen(error_msg), 0);
        return;
    }

    char file_content[8192] = {0};
    while (fgets(buffer, sizeof(buffer), guide_file_ptr)) {
        strcat(file_content, buffer);
    }
    fclose(guide_file_ptr);

    printf("요청된 섹션 번호: %d\n", guide_number);


    char *content_ptr = file_content;
    char *section_start = NULL;
    char *section_end = NULL;
    int found_section = 0;
    
    while (*content_ptr) {
        if (*content_ptr == '+') {
            int plus_count = 0;
            char *temp = content_ptr;
            while (*temp == '+') {
                plus_count++;
                temp++;
            }
            if (plus_count == guide_number) {
                section_start = content_ptr + plus_count;
                while (*section_start == ' ' || *section_start == '.') section_start++; 
                section_end = strstr(section_start, "//");
                found_section = 1;
                break;
            }
        }
        content_ptr++;
    }

    if (!found_section) {
        char error_msg[] = "요청하신 보험 안내 정보를 찾을 수 없습니다.\n";
        send(clientSock, error_msg, strlen(error_msg), 0);
        return;
    }

    size_t section_length = section_end ? (section_end - section_start) : strlen(section_start);
    char *current_pos = section_start;
    int total_sent = 0;
    int chunk_count = 0;
    
    while (section_length > 0) {
        size_t chunk_size = (section_length > 1024) ? 1024 : section_length;
        char chunk[1025];
        strncpy(chunk, current_pos, chunk_size);
        chunk[chunk_size] = '\0';
        
        int sent = send(clientSock, chunk, strlen(chunk), 0);
        if (sent == SOCKET_ERROR) {
            printf("전송 오류 발생\n");
            break;
        }
        
        total_sent += sent;
        chunk_count++;
        current_pos += chunk_size;
        section_length -= chunk_size;

        Sleep(10);
    }
    
    char end_marker[] = "\n=== 전송 완료 ===\n";
    send(clientSock, end_marker, strlen(end_marker), 0);
    
    printf("총 %d 청크, %d 바이트 전송 완료\n", chunk_count, total_sent);
}

void EXIT_PROGRAM(SOCKET clientSock) {
    send(clientSock, "프로그램을 종료합니다.\n", strlen("프로그램을 종료합니다.\n"), 0);
    closesocket(clientSock);
}

void handle_client_message(int clientSock, const char* msg, int* logged_in, char user_id[], size_t user_id_buf_size) {
    if (strncmp(msg, "lo/", 3) == 0) {
        char id[20] = {0}, pass[20] = {0}; 
        char* first_slash = strchr(msg + 3, '/');
        if (first_slash) {
            int id_len = first_slash - (msg + 3);
            strncpy(id, msg + 3, id_len);
            id[id_len] = '\0'; 

            strcpy(pass, first_slash + 1);
            pass[sizeof(pass) - 1] = '\0'; 

            printf("Received login attempt for ID='%s', PW='%s'\\n", id, pass); 

            if (UserOk(id, pass)) {
                *logged_in = 1;
                strncpy(user_id, id, user_id_buf_size - 1);
                user_id[user_id_buf_size - 1] = '\0';
                send(clientSock, "로그인 성공\\n", strlen("로그인 성공\\n"), 0);
                printf("사용자 로그인: %s\\n", id);
                
            } else {
                send(clientSock, "아이디 또는 비밀번호가 일치하지 않습니다.\\n", 
                     strlen("아이디 또는 비밀번호가 일치하지 않습니다.\\n"), 0);
                printf("로그인 실패: %s\\n", id);
            }
        } else {
            printf("Login message format error (missing slash): '%s'\\n", msg);
            send(clientSock, "로그인 요청 형식이 올바르지 않습니다.\\n", strlen("로그인 요청 형식이 올바르지 않습니다.\\n"), 0);
        }
    } else if (strncmp(msg, "new/", 4) == 0) {
        if (!(*logged_in)) {
            char register_msg[BUF_SIZE];
            strncpy(register_msg, msg + 4, sizeof(register_msg) - 1);
            register_msg[sizeof(register_msg) - 1] = '\0';

            char *temp_msg = register_msg;
            char *id = (char*)strtok_r(temp_msg, "/", &temp_msg);
            char *password = (char*)strtok_r(temp_msg, "/", &temp_msg);
            char *car_number = (char*)strtok_r(temp_msg, "/", &temp_msg);
            char *car_type = (char*)strtok_r(temp_msg, "/", &temp_msg);
            char *insurance_company = (char*)strtok_r(temp_msg, "/", &temp_msg);

            printf("회원가입 요청 - ID: %s, PW: %s, 차량번호: %s, 차종류: %s, 보험사: %s\n", 
                   id ? id : "NULL", password ? password : "NULL", 
                   car_number ? car_number : "NULL", car_type ? car_type : "NULL", 
                   insurance_company ? insurance_company : "NULL");

            if (id && password && car_number && car_type && insurance_company) {
                if (handle_new_user(id, password, car_number, car_type, insurance_company)) {
                    send(clientSock, "1\n", strlen("1\n"), 0); 
                    printf("회원가입 성공: %s\n", id);
                } else {
                    send(clientSock, "0\n", strlen("0\n"), 0); 
                    printf("회원가입 실패: %s (ID 중복 또는 파일 오류)\n", id);
                }
            } else {
                send(clientSock, "회원가입 정보 형식이 올바르지 않습니다.\n", 
                     strlen("회원가입 정보 형식이 올바르지 않습니다.\n"), 0);
                printf("회원가입 실패: 정보 형식 오류\n");
            }
        } else {
            send(clientSock, "이미 로그인되어 있습니다.\n", 
                 strlen("이미 로그인되어 있습니다.\n"), 0);
            printf("회원가입 실패: 이미 로그인된 사용자\n");
        }
    } else if (strncmp(msg, "check_id/", 9) == 0) {
     
        char check_id[20];
        strncpy(check_id, msg + 9, sizeof(check_id) - 1);
        check_id[sizeof(check_id) - 1] = '\0';
        
        printf("ID 중복 체크 요청 - ID: %s\n", check_id);
        
        FILE *file = fopen(USER_FILE, "r");
        int is_duplicate = 0;
        
        if (file) {
            printf("파일 열기 성공\n");
            char line[256];
            while (fgets(line, sizeof(line), file)) {
                line[strcspn(line, "\r\n")] = '\0';
                printf("파일에서 읽은 라인: %s\n", line);
                
                char *saveptr;
                char *current_id = strtok_r(line, "/", &saveptr);
                if (current_id && strcmp(current_id, check_id) == 0) {
                    is_duplicate = 1;
                    printf("ID 중복 발견 - %s\n", check_id);
                    break;
                }
            }
            fclose(file);
        } else {
            printf("파일 열기 실패 - %s\n", strerror(errno));
            printf("파일이 존재하지 않으므로 ID 사용 가능\n");
        }
        
        if (is_duplicate) {
            send(clientSock, "1\n", strlen("1\n"), 0); 
            printf("ID 중복 확인 - %s\n", check_id);
        } else {
            send(clientSock, "0\n", strlen("0\n"), 0); 
            printf("ID 사용 가능 - %s\n", check_id);
        }
    } else if (strncmp(msg, "AC", 2) == 0) {
        if (*logged_in) {
            show_accident_guide(clientSock);
        } else {
            send(clientSock, "로그인이 필요합니다.\n", strlen("로그인이 필요합니다.\n"), 0);
        }
    } else if (strncmp(msg, "LAW", 3) == 0) {
        if (*logged_in) {
            show_law_guide(clientSock);
        } else {
            send(clientSock, "로그인이 필요합니다.\n", strlen("로그인이 필요합니다.\n"), 0);
        }
    } else if (strncmp(msg, "BI/", 3) == 0) {
        if (*logged_in) {
            BI_GUIDE(clientSock, msg);
        } else {
            send(clientSock, "로그인이 필요합니다.\n", strlen("로그인이 필요합니다.\n"), 0);
        }
    } else if (strncmp(msg, "BS/", 3) == 0) {
        if (*logged_in) {
            BS_GUIDE(clientSock, msg);
        } else {
            send(clientSock, "로그인이 필요합니다.\n", strlen("로그인이 필요합니다.\n"), 0);
        }
    } else if (strncmp(msg, "AI/", 3) == 0) {
        if (*logged_in) {
            if (handle_ai_command(msg)) {
                send(clientSock, "사고 정보가 성공적으로 저장되었습니다.\n", 
                     strlen("사고 정보가 성공적으로 저장되었습니다.\n"), 0);
            } else {
                send(clientSock, "사고 정보 저장에 실패했습니다.\n", 
                     strlen("사고 정보 저장에 실패했습니다.\n"), 0);
            }
        } else {
            send(clientSock, "로그인이 필요합니다.\n", strlen("로그인이 필요합니다.\n"), 0);
        }
    } else if (strncmp(msg, "EX", 2) == 0) {
        EXIT_PROGRAM(clientSock);
    } else if (strncmp(msg, "UF", 2) == 0) {
        if (*logged_in) {
            if (USER_DATAUPDATE(msg, user_id)) {
                send(clientSock, "OK:사용자 정보가 성공적으로 업데이트되었습니다.\n", 
                     strlen("OK:사용자 정보가 성공적으로 업데이트되었습니다.\n"), 0);
            } else {
                send(clientSock, "오류:사용자 정보 업데이트에 실패했습니다.\n", 
                     strlen("오류:사용자 정보 업데이트에 실패했습니다.\n"), 0);
            }
        } else {
            send(clientSock, "로그인이 필요합니다.\n", strlen("로그인이 필요합니다.\n"), 0);
        }
    }
}

unsigned WINAPI HandleClient(void* arg) {

    SOCKET clientSock = *((SOCKET*)arg);
    int strLen = 0, i;
    char msg[BUF_SIZE];
    int logged_in = 0;
    char user_id[50] = {0};

    while (1) { 
        strLen = recv(clientSock, msg, sizeof(msg) - 1, 0); 
        
        if (strLen <= 0) { 
            if (strLen == 0) {
                printf("클라이언트 %s가 정상적으로 연결을 종료했습니다.\n", user_id[0] ? user_id : "미인증 사용자");
            } else {
                printf("클라이언트 %s와의 통신 오류 발생: %d\n", user_id[0] ? user_id : "미인증 사용자", WSAGetLastError());
            }
            break; 
        }

        msg[strLen] = '\0';
        printf("수신된 메시지: %s\n", msg);  

        handle_client_message(clientSock, msg, &logged_in, user_id, sizeof(user_id));
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
    printf("현재 연결된 클라이언트 수: %d\n", clientCount);
    ReleaseMutex(hMutex);
    closesocket(clientSock);
    return 0;
}


int main() {
    SetConsoleOutputCP(CP_UTF8); 
    SetConsoleCP(CP_UTF8); 
    
    char current_dir[256];
    if (GetCurrentDirectory(sizeof(current_dir), current_dir)) {
        printf("현재 작업 디렉토리: %s\n", current_dir);
    }
    printf("USER_FILE 경로: %s\n", USER_FILE);
    
    WSADATA wsaData;
    SOCKET serverSock, clientSock;
    SOCKADDR_IN serverAddr, clientAddr;
    int clientAddrSize;
    HANDLE hThread;
    HANDLE threadHandles[MAX_CLNT] = {0}; 

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
    printf("클라이언트 연결을 기다리는 중...\n");

    while (1) {
        clientAddrSize = sizeof(clientAddr);
        clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &clientAddrSize);
        
        if (clientSock == INVALID_SOCKET) {
            printf("클라이언트 연결 수락 실패: %d\n", WSAGetLastError());
            continue;
        }
        
        WaitForSingleObject(hMutex, INFINITE);

        if (clientCount < MAX_CLNT) {
            clientSocks[clientCount] = clientSock;
  
            hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)&clientSock, 0, NULL);
            if (hThread) {
                threadHandles[clientCount] = hThread;
                clientCount++;
                printf("연결된 클라이언트 IP: %s (총 %d명)\n", inet_ntoa(clientAddr.sin_addr), clientCount);
            } else {
                printf("스레드 생성 실패\n");
                closesocket(clientSock);
            }
        } else {
            printf("최대 클라이언트 수 초과\n");
            closesocket(clientSock);
        }
        
        ReleaseMutex(hMutex);

        WaitForSingleObject(hMutex, INFINITE);
        for (int i = 0; i < clientCount; i++) {
            if (threadHandles[i] && WaitForSingleObject(threadHandles[i], 0) == WAIT_OBJECT_0) {
                CloseHandle(threadHandles[i]);
                threadHandles[i] = NULL;
            }
        }
        ReleaseMutex(hMutex);
    }

    for (int i = 0; i < clientCount; i++) {
        if (threadHandles[i]) {
            CloseHandle(threadHandles[i]);
        }
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

int get_insurance_info(const char* message, char* response) {
    printf("함수 사용중....\n");
    if (!message) {
        printf("받은 메시지가 NULL\n");
        return 0;
    }

    char msg_copy[1024];
    strncpy(msg_copy, message, sizeof(msg_copy) - 1);
    msg_copy[sizeof(msg_copy) - 1] = '\0';
    
    char *token = strtok(msg_copy, "/");
    if (!token || strcmp(token, "BI") != 0) {
        printf("메시지 형식이 BI가 아님\n");
        return 0;
    }

    char *user_id = strtok(NULL, "/");
    if (!user_id) {
        printf("사용자 ID가 없음\n");
        return 0;
    }

    char *item_str = strtok(NULL, "/");
    if (!item_str) {
        printf("항목 번호가 없음\n");
        return 0;
    }
    int item = atoi(item_str);

    FILE *user_file = fopen(USER_FILE, "r");
    if (!user_file) {
        printf("사용자 정보 파일 열기 실패\n");
        return 0;
    }

    char line[256];
    char *insurance_company = NULL;
    int found = 0;

    while (fgets(line, sizeof(line), user_file)) {
        line[strcspn(line, "\n")] = 0;
        
        char *saveptr;
        char *current_id = strtok_r(line, "/", &saveptr);
        if (!current_id) continue;

        if (strcmp(current_id, user_id) == 0) {
  
            strtok_r(NULL, "/", &saveptr);
            strtok_r(NULL, "/", &saveptr);
            strtok_r(NULL, "/", &saveptr);
 
            insurance_company = strtok_r(NULL, "/", &saveptr);
            if (insurance_company) {
                found = 1;
                break;
            }
        }
    }
    fclose(user_file);

    if (!found || !insurance_company) {
        printf("사용자 또는 보험사 정보를 찾을 수 없음\n");
        return 0;
    }

    const char *insurance_file = NULL;
    if (strcmp(insurance_company, "삼성화재") == 0) {
        insurance_file = SAMSUNG_ACCIDENT_FILE;
    } else if (strcmp(insurance_company, "한화손해보험") == 0) {
        insurance_file = HANWHA_ACCIDENT_FILE;
    } else if (strcmp(insurance_company, "KB손해보험") == 0) {
        insurance_file = KB_ACCIDENT_FILE;
    } else if (strcmp(insurance_company, "DB손해보험") == 0) {
        insurance_file = DB_ACCIDENT_FILE;
    } else {
        printf("지원하지 않는 보험사\n");
        return 0;
    }

    FILE *info_file = fopen(insurance_file, "r");
    if (!info_file) {
        printf("보험사 정보 파일 열기 실패\n");
        return 0;
    }

    char file_content[10000] = {0};
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), info_file)) {
        trim_trailing_newlines(buffer);
        strcat(file_content, buffer);
        strcat(file_content, "\n");
    }
    fclose(info_file);

    printf("파일 내용: %s\n", file_content);

    char *start = file_content;
    char *end = NULL;
    int current_item = 0;
    int found_item = 0;

    while (*start) {

        if (*start == '+') {
            current_item++;
            printf("%d 발견\n", current_item);
            
            if (current_item == item) {
                found_item = 1;
                end = strchr(start + 1, '+');
                if (end) {

                    char temp = *end;
                    *end = '\0';
                    
                    snprintf(response, 4096, "BI/%s/%d/%s", user_id, item, start);
                    printf("내용: %s\n", start);
      
                    *end = temp;
                    return 1;
                } else {
                    snprintf(response, 4096, "BI/%s/%d/%s", user_id, item, start);
                    printf("추출된 내용(마지막 항목): %s\n", start);
                    return 1;
                }
            }
        }
        start++;
    }

    if (!found_item) {
        printf("해당 항목을 찾을 수 없음\n");
        return 0;
    }

    return 1;
}