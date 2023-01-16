#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for Project 3 */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000
#define MAXLINELENGTH 1000

typedef struct IFIDStruct {
    int instr;
    int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
    int instr;
    int pcPlus1;
    int readRegA;
    int readRegB;
    int offset;
} IDEXType;

typedef struct EXMEMStruct {
    int instr;
    int branchTarget;
    int eq;
    int aluResult;
    int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
    int instr;
    int writeData;
} MEMWBType;

typedef struct WBENDStruct {
    int instr;
    int writeData;
} WBENDType;

typedef struct stateStruct {
    int pc;
    int instrMem[NUMMEMORY];
    int dataMem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    IFIDType IFID;
    IDEXType IDEX;
    EXMEMType EXMEM;
    MEMWBType MEMWB;
    WBENDType WBEND;
    int cycles; /* number of cycles run so far */
} stateType;

int
field0(int instruction) // regA
{
    return((instruction >> 19) & 0x7);
}

int
field1(int instruction) // regB
{
    return((instruction >> 16) & 0x7);
}

int
field2(int instruction) // offset
{
    return(instruction & 0xFFFF);
}

int
opcode(int instruction)
{
    return(instruction >> 22);
}

void printInstruction(int instr) {
    switch (opcode(instr)) {
    case ADD:
        printf("add");
        break;
    case NOR:
        printf("nor");
        break;
    case LW:
        printf("lw");
        break;
    case SW:
        printf("sw");
        break;
    case BEQ:
        printf("beq");
        break;
    case JALR:
        printf("jalr");
        break;
    case HALT:
        printf("halt");
        break;
    case NOOP:
        printf("noop");
        break;
    default:
        printf(".fill %d", instr);
        return;
    }
    printf(" %d %d %d", field0(instr), field1(instr), field2(instr));
}

void printState(stateType* statePtr) {
    printf("\n@@@\n");
    printf("state before cycle %d starts:\n", statePtr->cycles);
    printf("\tpc = %d\n", statePtr->pc);

    printf("\tdata memory:\n");
    for (int i = 0; i < statePtr->numMemory; ++i) {
        printf("\t\tdataMem[ %d ] = %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (int i = 0; i < NUMREGS; ++i) {
        printf("\t\treg[ %d ] = %d\n", i, statePtr->reg[i]);
    }

    // IF/ID
    printf("\tIF/ID pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IFID.instr);
    printInstruction(statePtr->IFID.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IFID.pcPlus1);
    if (opcode(statePtr->IFID.instr) == NOOP) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // ID/EX
    int idexOp = opcode(statePtr->IDEX.instr);
    printf("\tID/EX pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IDEX.instr);
    printInstruction(statePtr->IDEX.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IDEX.pcPlus1);
    if (idexOp == NOOP) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegA = %d", statePtr->IDEX.readRegA);
    if (idexOp >= HALT || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->IDEX.readRegB);
    if (idexOp == LW || idexOp > BEQ || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\toffset = %d", statePtr->IDEX.offset);
    if (idexOp != LW && idexOp != SW && idexOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // EX/MEM
    int exmemOp = opcode(statePtr->EXMEM.instr);
    printf("\tEX/MEM pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->EXMEM.instr);
    printInstruction(statePtr->EXMEM.instr);
    printf(" )\n");
    printf("\t\tbranchTarget %d", statePtr->EXMEM.branchTarget);
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\teq ? %s", (statePtr->EXMEM.eq ? "True" : "False"));
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\taluResult = %d", statePtr->EXMEM.aluResult);
    if (exmemOp > SW || exmemOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->EXMEM.readRegB);
    if (exmemOp != SW) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // MEM/WB
    int memwbOp = opcode(statePtr->MEMWB.instr);
    printf("\tMEM/WB pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->MEMWB.instr);
    printInstruction(statePtr->MEMWB.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->MEMWB.writeData);
    if (memwbOp >= SW || memwbOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // WB/END
    int wbendOp = opcode(statePtr->WBEND.instr);
    printf("\tWB/END pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->WBEND.instr);
    printInstruction(statePtr->WBEND.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->WBEND.writeData);
    if (wbendOp >= SW || wbendOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    printf("end state\n");
}


int
convertNum(int num)
{
    /* converts a 16-bit number into a 32-bit Linux integer */
    if (num & (1 << 15)) {
        num -= (1 << 16);
    }
    return(num);
}

void initialize(stateType* state)
{
    state->pc = 0;
    state->cycles = 0;
    for (int i = 0; i < NUMREGS; i++) {
        state->reg[i] = 0;
    }
    state->IFID.instr = NOOPINSTRUCTION;
    state->IDEX.instr = NOOPINSTRUCTION;
    state->EXMEM.instr = NOOPINSTRUCTION;
    state->MEMWB.instr = NOOPINSTRUCTION;
    state->WBEND.instr = NOOPINSTRUCTION;
}

int stall_check(stateType* state) {
    int stall = 0;
    int instr = state->IFID.instr;
    if (opcode(state->IDEX.instr) == 2) {                   // instr ahead is lw
        if (field1(state->IDEX.instr) == field0(instr)) {
            stall = 1;
        }
        else if (field1(state->IDEX.instr) == field1(instr)) {
            stall = 1;
        }
    }
    return stall;
}

void wbend_check(stateType* state, int* regA, int* regB) {
    int wbend_instr = state->WBEND.instr;
    int idex_instr = state->IDEX.instr;
    if (field2(wbend_instr) == field0(idex_instr)) {
        *regA = state->WBEND.writeData;
    }
    if (field2(wbend_instr) == field1(idex_instr)) {
        *regB = state->WBEND.writeData;
    }
}

void memwb_check(stateType* state, int* regA, int* regB) {
    int memwb_instr = state->MEMWB.instr;
    int idex_instr = state->IDEX.instr;
    if (field2(memwb_instr) == field0(idex_instr)) {
        *regA = state->MEMWB.writeData;
    }
    if (field2(memwb_instr) == field1(idex_instr)) {
        *regB = state->MEMWB.writeData;
    }
}

void exmem_check(stateType* state, int* regA, int* regB) {
    int exmem_instr = state->EXMEM.instr;
    int idex_instr = state->IDEX.instr;
    if (field2(exmem_instr) == field0(idex_instr)) {
        *regA = state->EXMEM.aluResult;
    }
    if (field2(exmem_instr) == field1(idex_instr)) {
        *regB = state->EXMEM.aluResult;
    }
}