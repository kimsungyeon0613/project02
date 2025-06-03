#include <stdio.h>

typedef struct {
    char user_id[20]; // 사용자 아이디
    char date[20]; // 사고 발생 날짜
    char type[20]; // 사고 유형
    char detail[200]; // 사고 상세 내역
    char falut[10]; // 가해자/피해자
}ACCIDENT_DATA;

int main(void)
{   
    ACCIDENT_DATA accident;
    
    printf("==============================================\n");
    printf("               사고 정보 입력\n");
    printf("1.         언제 사고가 발생했나요?(15자 이내)\n");
    printf("2.         사고 유형은 무엇인가요?(30자 이내)\n");
    printf("3.         어떻게 사고가 발생했나요?(200자 이내)\n");
    printf("4.         가해자/피해자는 누구입니까?(가해자/피해자 하나 입력)\n");
    printf("==============================================\n");

    printf("\n사고 발생 날짜: ");
    scanf("%s", accident.date);
    
    printf("사고 유형: ");
    scanf("%s", accident.type);
    
    printf("사고 상세 내역: ");
    scanf(" %[^\n]s", accident.detail);
    
    printf("가해자/피해자: ");
    scanf("%s", accident.falut);

    printf("\n입력된 사고 정보:\n");
    printf("날짜: %s\n", accident.date);
    printf("유형: %s\n", accident.type);
    printf("상세내역: %s\n", accident.detail);
    printf("가해자/피해자: %s\n", accident.falut);

    return 0;
}
