#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LABEL 20
#define MAX_OPCODE 10
#define MAX_OPERAND 20
#define MAX_LINE 100
#define MAX_SYMBOLS 100
#define MAX_OPCODES 50

typedef struct {
    char label[MAX_LABEL];
    int address;
} SymbolTableEntry;

typedef struct {
    char mnemonic[MAX_OPCODE];
    char opcode[3];
} OpTableEntry;

typedef struct {
    char literal[MAX_OPERAND]; // 리터럴 값
    int address;               // 할당된 메모리 주소
} LiteralTableEntry;

LiteralTableEntry littab[MAX_SYMBOLS]; // 리터럴 테이블
int littabSize = 0;                   // 리터럴 테이블 크기


// 데이터 구조 선언
SymbolTableEntry symtab[MAX_SYMBOLS];
OpTableEntry optab[MAX_OPCODES];
int symtabSize = 0, optabSize = 0;

// Location Counter 초기화
int locctr = 0;
int startAddress = 0;

// OPTAB 로드
void loadOptab(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("OPTAB 파일을 열 수 없습니다.\n");
        exit(1);
    }

    while (fscanf(file, "%s %s", optab[optabSize].mnemonic, optab[optabSize].opcode) != EOF) {
        optabSize++;
    }

    fclose(file);
}

// OPTAB에서 명령어 찾기
int searchOptab(const char* mnemonic) {
    for (int i = 0; i < optabSize; i++) {
        if (strcmp(optab[i].mnemonic, mnemonic) == 0) {
            return i;
        }
    }
    return -1;
}

// SYMTAB에서 심볼 찾기
int searchSymtab(const char* label) {
    for (int i = 0; i < symtabSize; i++) {
        if (strcmp(symtab[i].label, label) == 0) {
            return i;
        }
    }
    return -1;
}

// SYMTAB에 심볼 추가
void addSymtab(const char* label, int address) {
    if (searchSymtab(label) == -1) {
        strcpy(symtab[symtabSize].label, label);
        symtab[symtabSize].address = address;
        symtabSize++;
    }
    else {
        printf("에러: 심볼 %s가 이미 SYMTAB에 존재합니다.\n", label);
    }
}

void pass1(const char* srcfile, const char* intfile) {
    FILE* src = fopen(srcfile, "r");
    FILE* intermediate = fopen(intfile, "w");

    if (!src || !intermediate) {
        printf("파일을 열 수 없습니다.\n");
        exit(1);
    }

    char line[MAX_LINE], label[MAX_LABEL], opcode[MAX_OPCODE], operand[MAX_OPERAND];

    // 첫 번째 줄 읽기
    fgets(line, sizeof(line), src);
    sscanf(line, "%s %s %s", label, opcode, operand);

    if (strcmp(opcode, "START") == 0) {
        startAddress = (int)strtol(operand, NULL, 16);
        locctr = startAddress;
        fprintf(intermediate, "%04X\t%s\t%s\t%s\n", locctr, label, opcode, operand);
        fgets(line, sizeof(line), src);
    }
    else {
        locctr = 0;
    }

    while (!feof(src)) {
        label[0] = '\0';
        opcode[0] = '\0';
        operand[0] = '\0';
        sscanf(line, "%s %s %s", label, opcode, operand);

        // 리터럴 처리
        if (operand[0] == '=') { // 리터럴인지 확인
            int exists = 0;
            for (int i = 0; i < littabSize; i++) {
                if (strcmp(littab[i].literal, operand) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists) {
                strcpy(littab[littabSize].literal, operand);
                littab[littabSize].address = -1; // 주소는 이후에 할당
                littabSize++;
            }
        }

        // SYMTAB에 라벨 추가
        if (label[0] != '-' && label[0] != '\0') {
            if (searchOptab(label) == -1 && strcmp(opcode, "START") != 0 &&
                strcmp(opcode, "END") != 0) {
                addSymtab(label, locctr);
            }
        }

        // 현재 행을 INTFILE에 기록
        fprintf(intermediate, "%04X\t%s\t%s\t%s\n", locctr, label, opcode, operand);

        // 명령어 크기 계산
        int optIndex = searchOptab(opcode);
        if (optIndex != -1) {
            locctr += 3;
        }
        else if (strcmp(opcode, "WORD") == 0) {
            locctr += 3;
        }
        else if (strcmp(opcode, "RESW") == 0) {
            locctr += 3 * atoi(operand);
        }
        else if (strcmp(opcode, "RESB") == 0) {
            locctr += atoi(operand);
        }
        else if (strcmp(opcode, "BYTE") == 0) {
            if (operand[0] == 'X') {
                locctr += (strlen(operand) - 3) / 2;
            }
            else if (operand[0] == 'C') {
                locctr += strlen(operand) - 3;
            }
        }
        else
			locctr += 3;

        fgets(line, sizeof(line), src);
    }

    // 리터럴을 프로그램 끝에 할당
    for (int i = 0; i < littabSize; i++) {
        if (littab[i].address == -1) { // 아직 할당되지 않은 리터럴
            littab[i].address = locctr;
            fprintf(intermediate, "%04X\t*\t\t%s\n", locctr, littab[i].literal);
            locctr += 3; // 리터럴 크기 (WORD 크기 가정)
        }
    }

    fclose(src);
    fclose(intermediate);
}


void pass2(const char* intfile, const char* objfile) {
    FILE* intermediate = fopen(intfile, "r");
    FILE* output = fopen(objfile, "w");

    if (!intermediate || !output) {
        printf("파일을 열 수 없습니다.\n");
        exit(1);
    }

    char line[MAX_LINE], label[MAX_LABEL], opcode[MAX_OPCODE], operand[MAX_OPERAND];
    int address;

    // Header Record 작성
    fprintf(output, "H%06X%06X\n", startAddress, locctr - startAddress);

    while (fgets(line, sizeof(line), intermediate)) {
        sscanf(line, "%X %s %s %s", &address, label, opcode, operand);

        int optIndex = searchOptab(opcode);
        if (optIndex != -1) {
            fprintf(output, "T%06X%02X\n", address, atoi(optab[optIndex].opcode));
        }
        else if (strcmp(opcode, "WORD") == 0) {
            fprintf(output, "T%06X%06X\n", address, atoi(operand));
        }
        else if (strcmp(opcode, "BYTE") == 0) {
            fprintf(output, "T%06X%02X\n", address, atoi(operand));
        }
        else if (operand[0] == '=') {
            // 리터럴 처리
            for (int i = 0; i < littabSize; i++) {
                if (strcmp(littab[i].literal, operand) == 0) {
                    fprintf(output, "T%06X%06X\n", address, littab[i].address);
                    break;
                }
            }
        }
    }

    // End Record 작성
    fprintf(output, "E%06X\n", startAddress);

    fclose(intermediate);
    fclose(output);
}


void writeSymtabToFile(const char* symfile) {
    FILE* file = fopen(symfile, "w");
    if (!file) {
        printf("SYMTAB 파일을 열 수 없습니다.\n");
        exit(1);
    }

    fprintf(file, "Symbol Table:\n");
    fprintf(file, "Label\t\tAddress\n");
    fprintf(file, "------------------------\n");

    for (int i = 0; i < symtabSize; i++) {
        fprintf(file, "%s\t\t%04X\n", symtab[i].label, symtab[i].address);
    }

    fclose(file);
    printf("SYMTAB이 %s 파일에 저장되었습니다.\n", symfile);
}

void writeLittabToFile(const char* littabFile) {
    FILE* file = fopen(littabFile, "w");
    if (!file) {
        printf("LITTAB 파일을 열 수 없습니다.\n");
        exit(1);
    }

    fprintf(file, "Literal Table:\n");
    fprintf(file, "Literal\t\tAddress\n");
    fprintf(file, "------------------------\n");

    for (int i = 0; i < littabSize; i++) {
        fprintf(file, "%s\t\t%04X\n", littab[i].literal, littab[i].address);
    }

    fclose(file);
    printf("LITTAB이 %s 파일에 저장되었습니다.\n", littabFile);
}


int main() {
    // OPTAB 로드
    loadOptab("optab.txt");

    // Pass 1 실행
    pass1("srcfile.txt", "intfile.txt");

    // SYMTAB을 파일로 출력
    writeSymtabToFile("symtab.txt");
    // 리터럴 테이블을 파일로 출력
    writeLittabToFile("littab.txt");

    // Pass 2 실행
    pass2("intfile.txt", "objfile.txt");

    return 0;
}