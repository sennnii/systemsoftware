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
    char literal[MAX_OPERAND]; // ���ͷ� ��
    int address;               // �Ҵ�� �޸� �ּ�
} LiteralTableEntry;

LiteralTableEntry littab[MAX_SYMBOLS]; // ���ͷ� ���̺�
int littabSize = 0;                   // ���ͷ� ���̺� ũ��


// ������ ���� ����
SymbolTableEntry symtab[MAX_SYMBOLS];
OpTableEntry optab[MAX_OPCODES];
int symtabSize = 0, optabSize = 0;

// Location Counter �ʱ�ȭ
int locctr = 0;
int startAddress = 0;

// OPTAB �ε�
void loadOptab(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("OPTAB ������ �� �� �����ϴ�.\n");
        exit(1);
    }

    while (fscanf(file, "%s %s", optab[optabSize].mnemonic, optab[optabSize].opcode) != EOF) {
        optabSize++;
    }

    fclose(file);
}

// OPTAB���� ��ɾ� ã��
int searchOptab(const char* mnemonic) {
    for (int i = 0; i < optabSize; i++) {
        if (strcmp(optab[i].mnemonic, mnemonic) == 0) {
            return i;
        }
    }
    return -1;
}

// SYMTAB���� �ɺ� ã��
int searchSymtab(const char* label) {
    for (int i = 0; i < symtabSize; i++) {
        if (strcmp(symtab[i].label, label) == 0) {
            return i;
        }
    }
    return -1;
}

// SYMTAB�� �ɺ� �߰�
void addSymtab(const char* label, int address) {
    if (searchSymtab(label) == -1) {
        strcpy(symtab[symtabSize].label, label);
        symtab[symtabSize].address = address;
        symtabSize++;
    }
    else {
        printf("����: �ɺ� %s�� �̹� SYMTAB�� �����մϴ�.\n", label);
    }
}

void pass1(const char* srcfile, const char* intfile) {
    FILE* src = fopen(srcfile, "r");
    FILE* intermediate = fopen(intfile, "w");

    if (!src || !intermediate) {
        printf("������ �� �� �����ϴ�.\n");
        exit(1);
    }

    char line[MAX_LINE], label[MAX_LABEL], opcode[MAX_OPCODE], operand[MAX_OPERAND];

    // ù ��° �� �б�
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

        // ���ͷ� ó��
        if (operand[0] == '=') { // ���ͷ����� Ȯ��
            int exists = 0;
            for (int i = 0; i < littabSize; i++) {
                if (strcmp(littab[i].literal, operand) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists) {
                strcpy(littab[littabSize].literal, operand);
                littab[littabSize].address = -1; // �ּҴ� ���Ŀ� �Ҵ�
                littabSize++;
            }
        }

        // SYMTAB�� �� �߰�
        if (label[0] != '-' && label[0] != '\0') {
            if (searchOptab(label) == -1 && strcmp(opcode, "START") != 0 &&
                strcmp(opcode, "END") != 0) {
                addSymtab(label, locctr);
            }
        }

        // ���� ���� INTFILE�� ���
        fprintf(intermediate, "%04X\t%s\t%s\t%s\n", locctr, label, opcode, operand);

        // ��ɾ� ũ�� ���
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

    // ���ͷ��� ���α׷� ���� �Ҵ�
    for (int i = 0; i < littabSize; i++) {
        if (littab[i].address == -1) { // ���� �Ҵ���� ���� ���ͷ�
            littab[i].address = locctr;
            fprintf(intermediate, "%04X\t*\t\t%s\n", locctr, littab[i].literal);
            locctr += 3; // ���ͷ� ũ�� (WORD ũ�� ����)
        }
    }

    fclose(src);
    fclose(intermediate);
}


void pass2(const char* intfile, const char* objfile) {
    FILE* intermediate = fopen(intfile, "r");
    FILE* output = fopen(objfile, "w");

    if (!intermediate || !output) {
        printf("������ �� �� �����ϴ�.\n");
        exit(1);
    }

    char line[MAX_LINE], label[MAX_LABEL], opcode[MAX_OPCODE], operand[MAX_OPERAND];
    int address;

    // Header Record �ۼ�
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
            // ���ͷ� ó��
            for (int i = 0; i < littabSize; i++) {
                if (strcmp(littab[i].literal, operand) == 0) {
                    fprintf(output, "T%06X%06X\n", address, littab[i].address);
                    break;
                }
            }
        }
    }

    // End Record �ۼ�
    fprintf(output, "E%06X\n", startAddress);

    fclose(intermediate);
    fclose(output);
}


void writeSymtabToFile(const char* symfile) {
    FILE* file = fopen(symfile, "w");
    if (!file) {
        printf("SYMTAB ������ �� �� �����ϴ�.\n");
        exit(1);
    }

    fprintf(file, "Symbol Table:\n");
    fprintf(file, "Label\t\tAddress\n");
    fprintf(file, "------------------------\n");

    for (int i = 0; i < symtabSize; i++) {
        fprintf(file, "%s\t\t%04X\n", symtab[i].label, symtab[i].address);
    }

    fclose(file);
    printf("SYMTAB�� %s ���Ͽ� ����Ǿ����ϴ�.\n", symfile);
}

void writeLittabToFile(const char* littabFile) {
    FILE* file = fopen(littabFile, "w");
    if (!file) {
        printf("LITTAB ������ �� �� �����ϴ�.\n");
        exit(1);
    }

    fprintf(file, "Literal Table:\n");
    fprintf(file, "Literal\t\tAddress\n");
    fprintf(file, "------------------------\n");

    for (int i = 0; i < littabSize; i++) {
        fprintf(file, "%s\t\t%04X\n", littab[i].literal, littab[i].address);
    }

    fclose(file);
    printf("LITTAB�� %s ���Ͽ� ����Ǿ����ϴ�.\n", littabFile);
}


int main() {
    // OPTAB �ε�
    loadOptab("optab.txt");

    // Pass 1 ����
    pass1("srcfile.txt", "intfile.txt");

    // SYMTAB�� ���Ϸ� ���
    writeSymtabToFile("symtab.txt");
    // ���ͷ� ���̺��� ���Ϸ� ���
    writeLittabToFile("littab.txt");

    // Pass 2 ����
    pass2("intfile.txt", "objfile.txt");

    return 0;
}