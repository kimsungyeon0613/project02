#include <stdio.h>
#include <windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    printf("===========================\n");
    printf("===== 프로그램 기능 안내 =====\n");
    printf("1. 사용자정보 수정 기능\n");
    printf("2. 사고정보 입력 기능\n");
    printf("3. 보험사별 사고 유형 안내\n");
    printf("4. 보험사별 보상 안내\n");
    printf("5. 사고대처 방법 안내\n");
    printf("6. 12대 중과실 법률 안내\n");
    printf("7. 프로그램 종료\n");

    printf("===========================\n");
    printf("원하는 메뉴(번호)를 선택하세요: ");
    return 0;
}
