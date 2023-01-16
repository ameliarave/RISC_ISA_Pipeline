#include "simulator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma warning(disable : 4996)

int
main(int argc, char* argv[])
{
    // read in machine code file
    char* line = (char*)malloc(MAXLINELENGTH);
    static stateType state;
    FILE* filePtr;
    static stateType newState;

    int size = sizeof(state.instrMem) / sizeof(state.instrMem[0]);

    for (int i = 0; i < size; i++) {
        state.instrMem[i] = 0;
        newState.instrMem[i] = 0;
        state.dataMem[i] = 0;
        newState.dataMem[i] = 0;
    }

    if (argc != 2) {
        printf("error: usage: %s <machine-code-file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    int fill = 0;
    state.numMemory = fill;
    newState.numMemory = fill;
    while (fgets(line, MAXLINELENGTH, filePtr)) {   // read in machine code values to memory
        line[strcspn(line, "\n")] = 0;              // remove trailing newline
        sscanf(line, "%d", state.dataMem + fill);   // read number as size of unsigned int
        state.instrMem[fill] = state.dataMem[fill];
        newState.dataMem[fill] = state.dataMem[fill];
        newState.instrMem[fill] = state.dataMem[fill];
        state.numMemory++;
        newState.numMemory++;
        fill++;
    }
    free(line);
    line = NULL;
    initialize(&state);
    initialize(&newState);

    printf("instruction memory:\n");
    for (int i = 0; i < state.numMemory; i++) {
        printf("\tinstrMem[ %d ] = ", i);
        printInstruction(state.instrMem[i]);
        printf("\n");
    }

    while (opcode(state.MEMWB.instr) != HALT) {                         // Exits when HALT instr in MEMWB reg
        printState(&state);        
        newState = state;                                               // Starting a new cycle
        newState.cycles++;

        int ifid_instr = state.IFID.instr;
        int idex_instr = state.IDEX.instr;
        int exmem_instr = state.EXMEM.instr;
        int memwb_instr = state.MEMWB.instr;
        int wbend_instr = state.WBEND.instr;

        /* -------------------------------------------------------  START FETCH */
            
        newState.IFID.instr = state.instrMem[state.pc];
        newState.IFID.pcPlus1 = state.pc + 1;

        newState.IDEX.instr = ifid_instr;
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;

        // [End Fetch]...

        /* ---------------------END IF stage ---------------------  START DECODE */    

        int stall = 0;

        if (opcode(ifid_instr) == 0 || opcode(ifid_instr) == 1) {       // Add & Nor
            stall = stall_check(&state);
        }
        else if (opcode(ifid_instr) == 2) { // lw
            stall = stall_check(&state);
        }
        else if (opcode(ifid_instr) == 3) { // sw
            stall = stall_check(&state);
        }
        else if (opcode(ifid_instr) == 4) { // beq
            stall = stall_check(&state);
        }

        if (stall) {
            newState.pc--;
            newState.IDEX.instr = NOOPINSTRUCTION;
            newState.IFID.instr = ifid_instr;
            newState.IFID.pcPlus1 = state.IFID.pcPlus1;
        }

        newState.IDEX.readRegA = state.reg[field0(ifid_instr)];         // read reg[regA]
        newState.IDEX.readRegB = state.reg[field1(ifid_instr)];         // read reg[regB]

        if (opcode(ifid_instr) == 2 || opcode(ifid_instr) == 3 || opcode(ifid_instr) == 4) { // Add/Nor
            newState.IDEX.offset = convertNum(field2(ifid_instr));
        }  
       
        newState.EXMEM.instr = idex_instr;
        
        /* --------------------- END ID stage ---------------------  START EXECUTE */   

        int regA = state.IDEX.readRegA;
        int regB = state.IDEX.readRegB;        

        if (opcode(idex_instr) == 0 || opcode(idex_instr) == 1) {       // [IDEX]: ADD / NOR:
            
            if (opcode(wbend_instr) == 0 || opcode(wbend_instr) == 1) { // WBEND: Add / Nor
                wbend_check(&state, &regA, &regB);
            }
            else if (opcode(wbend_instr) == 2) {                        // WBEND: Lw
                if (field1(wbend_instr) == field0(idex_instr)) {
                    regA = state.WBEND.writeData;
                }
                if (field1(wbend_instr) == field1(idex_instr)) {
                    regB = state.WBEND.writeData;
                }
            }   // [ADD / NOR / LW] : WEBEND
            if (opcode(memwb_instr) == 0 || opcode(memwb_instr) == 1) { // MEMWB: Add/ Nor
                memwb_check(&state, &regA, &regB);
            }
            else if (opcode(memwb_instr) == 2) {
                if (field1(memwb_instr) == field0(idex_instr)) {
                    regA = state.MEMWB.writeData;
                }
                if (field1(memwb_instr) == field1(idex_instr)) {
                    regB = state.MEMWB.writeData;
                }
            }   // [ADD / NOR / LW] : MEMWB
            if (opcode(exmem_instr) == 0 || opcode(exmem_instr) == 1) { // EXMEM: Add / NOR
                exmem_check(&state, &regA, &regB);
            }   // [ADD / NOR]: EXEMEM

            if (opcode(idex_instr) == 0) {                              // UPDATE: Add
                newState.EXMEM.aluResult = regA + regB;
            }
            else {                                                      // UPDATE: Nor
                newState.EXMEM.aluResult = ~(regA | regB);
            }            
        }
        else if (opcode(idex_instr) == 2) {                             // [IDEX]: LW
            if (opcode(wbend_instr) == 0 || opcode(wbend_instr) == 1) { // WBEND: Add / Nor TODO CHECK THIS
                if (field2(wbend_instr) == field0(idex_instr)) {
                    regA = state.WBEND.writeData;
                }
            }
            else if (opcode(wbend_instr) == 2) {                        // WBEND: Lw
                if (field1(wbend_instr) == field0(idex_instr)) {
                    regA = state.WBEND.writeData;
                }
            }   // [ADD / NOR / LW]: WBEND
            if (opcode(memwb_instr) == 0 || opcode(memwb_instr) == 1) { // MEMWB: Add /Nor
                if (field2(memwb_instr) == field0(idex_instr)) {
                    regA = state.MEMWB.writeData;
                }
            }
            else if (opcode(memwb_instr) == 2) {                        // MEMWB: Lw
                if (field1(memwb_instr) == field0(idex_instr)) {
                    regA = state.MEMWB.writeData;
                }
            }   // [ADD / NOR / LW]: MEMWB
            if (opcode(exmem_instr) == 0 || opcode(exmem_instr) == 1) { // EXMEM: Add / Nor
                if (field2(exmem_instr) == field0(idex_instr)) {
                    regA = state.EXMEM.aluResult;
                }
            }   // [ADD / NOR]: EXMEM
            newState.EXMEM.aluResult = regA + state.IDEX.offset;
        }
        else if (opcode(idex_instr) == 3) {                             // [IDEX]: SW
            if (opcode(wbend_instr) == 0 || opcode(wbend_instr) == 1) { // WBEND: Add / Nor
                wbend_check(&state, &regA, &regB);
            }
            else if (opcode(wbend_instr) == 2) {                        // WBEND: Lw
                if (field1(wbend_instr) == field0(idex_instr)) {
                    regA = state.WBEND.writeData;
                }
                if (field1(wbend_instr) == field1(idex_instr)) {
                    regB = state.WBEND.writeData;
                }
            }   // [ADD / NOR / LW]: WBEND
            if (opcode(memwb_instr) == 0 || opcode(memwb_instr) == 1) { // MEMWB: Add / Nor
                memwb_check(&state, &regA, &regB);
            }
            else if (opcode(memwb_instr) == 2) {                        // MEMWB: Lw
                if (field1(memwb_instr) == field0(idex_instr)) {
                    regA = state.MEMWB.writeData;
                }
                if (field1(memwb_instr) == field1(idex_instr)) {
                    regB = state.MEMWB.writeData;
                }
            }   // [ADD / NOR / LW]: MEMWB
            if (opcode(exmem_instr) == 0 || opcode(exmem_instr) == 1) { // EXMEM: Add / Nor
                exmem_check(&state, &regA, &regB);
            }   // [ADD / NOR]: EXMEM
            newState.EXMEM.aluResult = regA + state.IDEX.offset;
            newState.EXMEM.readRegB = regB;
        }
        else if (opcode(idex_instr) == 4) {                             // BEQ:
            if (opcode(wbend_instr) == 0 || opcode(wbend_instr) == 1) { // WBEND: Add / Nor
                wbend_check(&state, &regA, &regB);
            }
            else if (opcode(wbend_instr) == 2) {                        // WBEND: Lw
                if (field1(wbend_instr) == field0(idex_instr)) {
                    regA = state.WBEND.writeData;
                }
                if (field1(wbend_instr) == field1(idex_instr)) {
                    regB = state.WBEND.writeData;
                }
            }   // [ADD / NOR / LW]: WBEND
            if (opcode(memwb_instr) == 0 || opcode(memwb_instr) == 1) { // MEMWB: Add / Nor
                memwb_check(&state, &regA, &regB);
            }
            else if (opcode(memwb_instr) == 2) {                        // MEMWB: Lw
                if (field1(memwb_instr) == field0(idex_instr)) {
                    regA = state.MEMWB.writeData;
                }
                if (field1(memwb_instr) == field1(idex_instr)) {
                    regB = state.MEMWB.writeData;
                }
            }   // [ADD / NOR / LW]: MEMWB
            if (opcode(exmem_instr) == 0 || opcode(exmem_instr) == 1) { // EXMEM: Add/Nor
                exmem_check(&state, &regA, &regB);
            }   // [ADD /NOR]: EXMEM
            if (regA == regB) {
                newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;
                newState.EXMEM.eq = 1;
            }   // else { dont care }
            newState.EXMEM.readRegB = regB;
        }
        newState.MEMWB.instr = state.EXMEM.instr;                      

        /* --------------------- END EX stage ---------------------  START MEMORY */
        
        if (opcode(exmem_instr) == 0 || opcode(exmem_instr) == 1) {
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        }
        else if (opcode(exmem_instr) == 2) {
            newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
        }
        else if (opcode(exmem_instr) == 3) {
            newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;
        }
        else if (opcode(exmem_instr) == 4) { // Branch
            if (state.EXMEM.eq == 1) {
                newState.IFID.pcPlus1 = state.EXMEM.branchTarget;
                newState.pc = state.EXMEM.branchTarget - 1;
                newState.IFID.instr = NOOPINSTRUCTION;
                newState.IDEX.instr = NOOPINSTRUCTION;
                newState.EXMEM.instr = NOOPINSTRUCTION;
            }
        }

        newState.WBEND.instr = state.MEMWB.instr;
        newState.WBEND.writeData = state.MEMWB.writeData;

        /* --------------------- END MEM stage ---------------------  START WRITEBACK */

        if (opcode(memwb_instr) == 0 || opcode(memwb_instr) == 1) {
            newState.reg[field2(memwb_instr)] = state.MEMWB.writeData;
        }
        else if (opcode(memwb_instr) == 2) {
            newState.reg[field1(memwb_instr)] = state.MEMWB.writeData;
        }
        // No other writeback instructions       

        /* --------------------- END WB stage ---------------------  */

        newState.pc++;
        state = newState; 
        /* End of the cycle. Update the current state with the values calculated in this cycle. */
    } 

    printf("machine halted\n");
    printf("total of %d cycles executed\n", state.cycles);
    printf("final state of machine:\n");
    printState(&state);

    return 0;
}   // Main
