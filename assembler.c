/** 
 * Assembler code fragment for LC-2K 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXLINELENGTH 1000
#define MAXLABELS 60
#define MAXCHARS 7

int getLabelVal(int*, char [MAXLABELS][MAXCHARS], int, char*);
int setRegA(char*);
int setRegB(char*);
int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int
main(int argc, char *argv[])
{

    int label_val[MAXLABELS];
    char label_str[MAXLABELS][MAXCHARS];
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    int line = 0;               /* keeps track of value to assign label*/
    int i = 0;                  /* index where this label's value will be stored sequentially */
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {  /*Pass 1*/
        if (strcmp(label, "") != 0) { 
            if (strlen(label) > 6 || !isalpha(label[0])) {  
                exit(1);
            }
            for (int j = 0; j < i; j++) {
                if (!strcmp(label_str[j], label)) {     /* checking for duplicate label declaration */
                    exit(1);
                }
            }
            label_val[i] = line;
            strcpy(label_str[i], label);        /* When label found, index using label[i] where found. */            
            i++;
        }
        line++;
    }    
    rewind(inFilePtr);   
    int pc = 0;
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) { /*Pass 2*/
        int mc = 0b0;
        int set = 0b0;
        if (!strcmp(opcode, "lw")) {          
            set = 0b010;              /* opcode 010 for lw */
            mc |= set << 22;          /* set opcode bits 24-22 */
            mc |= setRegA(arg0);      /* setting regA in 21-19 */
            mc |= setRegB(arg1);      /* setting regB in 18 - 16 */
            if (!isNumber(arg2)) {    /* symbolic label is being used for offsetfield, lookup value */
                set = getLabelVal(label_val, label_str, i, arg2);                                
            }
            else {
                set = atoi(arg2);
                if (set > 32767 || set < -32768) {
                    exit(1);            /* doesn't fit in 16-bit integer */
                }
            }
            if (set < 0) {
                int mask = 0b1111111111111111;
                set = set & mask;
            }
            mc |= set;                  /* setting offsetfield bits 15-0 */
        }
        else if (!strcmp(opcode, "sw")) {
            set = 0b011;
            mc |= set << 22;            /* set opcode bits 24-22 */
            mc |= setRegA(arg0);        /* set regA bits 21-19 */
            mc |= setRegB(arg1);        /* set regB 18-16 */
            if (!isNumber(arg2)) {      /* check if arg2 is a label and handle if so */
                set = getLabelVal(label_val, label_str, i, arg2);
            }
            else {                      /* arg2 not a label, set hard value in 15-0 */
                set = atoi(arg2);
                if (set > 32767 || set < -32768) {
                    exit(1);            /* doesn't fit in 16-bit integer */
                }
            }
            if (set < 0) {
                int mask = 0b1111111111111111;
                set = set & mask;
            }
            mc |= set;
        }
        else if (!strcmp(opcode, "beq")) {           
            set = 0b100;
            mc |= set << 22;
            mc |= setRegA(arg0);
            mc |= setRegB(arg1);
            if (!isNumber(arg2)) {          /* check if arg2 is a label and handle if so */
                set = getLabelVal(label_val, label_str, i, arg2);
                set = set - pc - 1;         /* offset = addrLabel - pc -1 */
            }
            else {                          /* arg2 not a label, set hard value in 15-0 */
                set = atoi(arg2);
            }
            if (set > 32767 || set < -32768) {
                exit(1);        /* doesn't fit in 16-bit integer */
            }
            if (set < 0) {
                int mask = 0b1111111111111111;
                set = set & mask;
            }            
            mc |= set;
        }
        else if (!strcmp(opcode, "add")) {
            /* opcode, regA, regB, destReg */
            set = 0b000;
            mc |= set << 22;
            mc |= setRegA(arg0);
            mc |= setRegB(arg1);
            mc |= atoi(arg2);       /* no need to shift for 15-0 desReg */
        }
        else if (!strcmp(opcode, "nor")) {
            set = 0b001;
            mc |= set << 22;
            mc |= setRegA(arg0);
            mc |= setRegB(arg1);
            mc |= atoi(arg2);
        }
        else if (!strcmp(opcode, "jalr")) { /* opcode, regA, regB */            
            set = 0b101;
            mc |= set << 22;
            mc |= setRegA(arg0);
            mc |= setRegB(arg1);
        }
        else if(!strcmp(opcode, "halt")){  /* opcode */            
            set = 0b110;
            mc |= set << 22;
        }
        else if (!strcmp(opcode, "noop")) { /* opcode */
            set = 0b111;
            mc |= set << 22;
        }
        else if (!strcmp(opcode, ".fill")){     /*.fill directive */
            if (!isNumber(arg0)) {              /* check if value is label and resolve label address */
                set = getLabelVal(label_val, label_str, i, arg0);
            }
            else {
                set = atoi(arg0);
            }
            mc |= set;
        }
        else {            
            exit(1);    /* undefined opcode */
        }
        pc++;
        fprintf(outFilePtr, "%i\n", mc);
    }
    return(0);
}


int
getLabelVal(int *vals, char label_str[MAXLABELS][MAXCHARS], int size_vals, char* arg)
{
    int set = 0;
    for (int j = 0; j < size_vals; j++) {
        if (!strcmp(arg, *label_str)) {     /* if this label found in label_str array */
            set = *vals;                    /* should retrieve value of this label */
            return set;
        }
        vals++;
        label_str++;
    }
    return set;
}

int
setRegA(char* arg)
{
    int set = atoi(arg) << 19;
    return set;
}

int
setRegB(char* arg)
{
    int set = atoi(arg) << 16;
    return set;
}

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
	/* reached end of file */
        return(0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH-1) {
	printf("error: line too long\n");
	exit(1);
    }

    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
	/* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);
    return(1);
}

int
isNumber(char *string)
{
    int i;
    return( (sscanf(string, "%d", &i)) == 1);
}