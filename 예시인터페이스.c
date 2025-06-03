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
    printf("==============================================\n");
    printf("               사고 정보 입력\n");
    printf("1.         언제 사고가 발생했나요?(15자 이내)\n");
    printf("2.         사고 유형은 무엇인가요?(30자 이내)\n");
    printf("3.         어떻게 사고가 발생했나요?(200자 이내)\n");
    printf("4.         가해자/피해자는 누구입니까?(가해자/피해자 하나 입력)\n");
    printf("==============================================\n");
}
