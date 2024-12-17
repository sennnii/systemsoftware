#include <iostream>
#include <cstring>
#include <iomanip>
#include <bitset>
#include <cstdlib>

using namespace std;

//Registerable 인터페이스
class Registerable {
public:
    virtual void load(int value) = 0;
    virtual int getValue() const = 0;
};

//Register 클래스
class Register : public Registerable {
protected:
    int value;

public:
    Register() : value(0) {}

    void load(int val) override {
        value = val;
    }

    int getValue() const override {
        return value;
    }
};

//PC 클래스
class PC : public Register {
public:
    void increment() {
        load(getValue() + 1);
    }
};

//ALU 클래스
class ALU : public Register {
public:
    void add(int a, int b) {
        load(a + b);
    }

    void mul(int a, int b) {
        load(a * b);
    }

    void div(int a, int b) {
        if (b != 0) load(a / b);
        else cerr << "Error: Division by zero!" << endl;
    }

    void mod(int a, int b) {
        if (b != 0) load(a % b);
        else cerr << "Error: Modulo by zero!" << endl;
    }
};

//CU 클래스
class CU {
public:
    void controlSignal(const string& operation) {
        cout << "Control Signal: Executing " << operation << endl;
    }
};

//CPU16 클래스
class CPU16 {
private:
    Register AC;
    ALU alu;
    CU cu;
    PC pc;
    int IR;

public:
    CPU16() : IR(0) {}

    void executeInstruction(int instruction, int* memory);
    void displayAC() const;
    void displayPC() const;
};

void CPU16::executeInstruction(int instruction, int* memory) {
    IR = instruction;
    int opcode = (instruction >> 12) & 0xF;
    int operand = instruction & 0xFFF;

    switch (opcode) {
    case 0x0:
        cu.controlSignal("LDA");
        AC.load(memory[operand]);
        break;
    case 0xF:
        cu.controlSignal("SEA");
        AC.load(operand);
        break;
    case 0x2:
        cu.controlSignal("ADD");
        alu.add(AC.getValue(), operand);
        AC.load(alu.getValue());
        break;
    case 0x3:
        cu.controlSignal("MUL");
        alu.mul(AC.getValue(), operand);
        AC.load(alu.getValue());
        break;
    case 0x4:
        cu.controlSignal("DIV");
        alu.div(AC.getValue(), operand);
        AC.load(alu.getValue());
        break;
    case 0x5:
        cu.controlSignal("MOD");
        alu.mod(AC.getValue(), operand);
        AC.load(alu.getValue());
        break;
    case 0x1:
        cu.controlSignal("STA");
        memory[operand] = AC.getValue();
        break;
    default:
        cerr << "Error: Invalid opcode!" << endl;
        break;
    }

    pc.increment();//명령어 실행 후 PC 증가
}

void CPU16::displayAC() const {
    cout << "AC: " << AC.getValue() << endl;
}

void CPU16::displayPC() const {
    cout << "PC: " << pc.getValue() << endl;
}

//Memory 클래스
class Memory {
public:
    int mem[4096];
    unsigned int mpt;

    Memory() : mpt(0) { reset(); }

    //특정 주소에서 16비트 읽기
    int read(unsigned int address) {
        if (address < 4096) return mem[address];
        cerr << "Error: Read address out of bounds!" << endl;
        return 0;
    }

    //현재 포인터에서 16비트 읽기
    int readAtPointer() {
        return read(mpt);
    }

    //메모리 첫 주소에서 16비트 읽기
    int readAtStart() {
        return read(0);
    }

    //특정 주소에 16비트 쓰기
    void write(unsigned int address, int value) {
        if (address < 4096) mem[address] = value;
        else cerr << "Error: Write address out of bounds!" << endl;
    }

    //현재 포인터에 16비트 쓰기
    void writeAtPointer(int value) {
        write(mpt, value);
    }

    //메모리 첫 주소에 16비트 쓰기
    void writeAtStart(int value) {
        write(0, value);
    }

    //메모리 전체 초기화 (0으로)
    void reset() {
        memset(mem, 0, sizeof(mem));
    }

    //특정 주소 범위 초기화 (0으로)
    void resetRange(unsigned int start, unsigned int end) {
        if (start < 4096 && end < 4096 && start <= end) {
            for (unsigned int i = start; i <= end; ++i) mem[i] = 0;
        }
        else {
            cerr << "Error: Invalid range for reset!" << endl;
        }
    }

    //메모리 전체를 임의의 값으로 초기화
    void randomize() {
        for (int i = 0; i < 4096; ++i) mem[i] = rand() % 65536;
    }

    //포인터 설정
    void setPointer(unsigned int address) {
        if (address < 4096) mpt = address;
        else cerr << "Error: Pointer out of bounds!" << endl;
    }

    //포인터 초기화 (0으로)
    void resetPointer() {
        mpt = 0;
    }

    //메모리 내용 보기 (16진수 또는 2진수)
    void displayMemory(int start, int end, bool binary = false) const {
        if (start < 0 || end >= 4096 || start > end) {
            cerr << "Error: Invalid range for display!" << endl;
            return;
        }
        for (int i = start; i <= end; ++i) {
            cout << "Address " << i << ": ";
            if (binary) {
                cout << bitset<16>(mem[i]) << endl;
            }
            else {
                cout << hex << setw(4) << setfill('0') << mem[i] << endl;
            }
        }
    }
};


//메인 함수
int main() {
    CPU16 cpu;
    Memory memory;

    //필수 프로그램 로드
    memory.write(0, 0xF7EF);  //SEA 7EF -> 0, 1번 주소
    memory.write(2, 0x2004);  //ADD 4   -> 2, 3번 주소
    memory.write(4, 0x3040);  //MUL 40  -> 4, 5번 주소
    memory.write(6, 0x1008);  //STA 8   -> 6, 7번 주소

    cout << "=== 필수 프로그램 실행 ===" << endl;
    for (int i = 0; i <= 6; i += 2) {
        cpu.executeInstruction(memory.read(i), memory.mem);
        cpu.displayAC();
        cpu.displayPC();
    }

    memory.displayMemory(0, 10);


    //선택 프로그램 로드
    memory.write(8, 0xF064);   //SEA 64  -> 8, 9번 주소
    memory.write(10, 0x4003);  //DIV 3   -> 10, 11번 주소
    memory.write(12, 0x5007);  //MOD 7   -> 12, 13번 주소
    memory.write(14, 0x1009);  //STA 9   -> 14, 15번 주소

    cout << "\n=== 선택 프로그램 실행 ===" << endl;
    for (int i = 8; i <= 14; i += 2) {
        cpu.executeInstruction(memory.read(i), memory.mem);
        cpu.displayAC();
        cpu.displayPC();
    }

    memory.displayMemory(0, 16);


    //메모리 동작 테스트
    cout << "\n=== 메모리 동작 테스트 ===" << endl;
    memory.setPointer(5);
    memory.writeAtPointer(1234);
    cout << "Memory at pointer (5): " << memory.readAtPointer() << endl;

    memory.resetRange(0, 10);
    cout << "\nAfter resetting range 0-10:" << endl;
    memory.displayMemory(0, 10);

    memory.randomize();
    cout << "\nAfter randomizing memory:" << endl;
    memory.displayMemory(0, 10, true);

    return 0;
}
