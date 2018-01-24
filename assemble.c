#include <stdio.h>
#include <string.h>
#include "globals.h"

#define DEBUG_ASSEMBLER 0


	/*
	** Parses program file and assembles (converts from assembler
	** to machine code).
	*/

void AssembleSimpleDLX(char *filename,		/* filename of program */
         struct instruction code[MAX_LINES_OF_CODE],  /* assembled code */
                        int *code_length)	/* #lines in program */

{
FILE	*fpt;
char	input[81],line[81],*field1,*field2,*field3,*oper1,*oper2,*oper3;
char	opcode[20],operands[40],label[20];
char	code_labels[MAX_LINES_OF_CODE][20],label_fields[MAX_LINES_OF_CODE][20];
int	inst_count,i,j;


if ((fpt=fopen(filename,"r")) == NULL)
  {
  printf("Unable to open %s for reading\n",filename);
  exit(0);
  }
	/* read and parse lines from file one line at a time */
inst_count=0;
if (DEBUG_ASSEMBLER)
  printf("PARSING:\n");
while(fgets(input,80,fpt) != NULL)
  {
	/* parse line into fields (text separated by whitespace) */
  strcpy(line,input);
  ParseLineIntoTokens(line," \t\n",&field1,&field2,&field3);
  if (field2 == NULL)
    {
    printf("Too few fields on the following line:\n");
    printf("%s",input);
    exit(0);
    }
  if (field3 != NULL)
    {	/* program line had label */
    strcpy(opcode,field2);
    strcpy(operands,field3);
    strcpy(label,field1);
    }
  else
    {	/* program line did not have label */
    strcpy(opcode,field1);
    strcpy(operands,field2);
    strcpy(label,"");
    }
  if (DEBUG_ASSEMBLER)
    printf("%s\t%s\t%s",(label == NULL ? "   " : label),opcode,operands);
	/* parse operands field into individual operands */
  ParseLineIntoTokens(operands,",",&oper1,&oper2,&oper3);
  if (DEBUG_ASSEMBLER)
    printf("\t\t%s\t%s\t%s\n",(oper1 == NULL ? "   " : oper1),
		(oper2 == NULL ? "   " : oper2),
		(oper3 == NULL ? "   " : oper3));
	/* add instruction to machine code, deciphering operands */
  if (strcmp(opcode,"ADDI") == 0  ||  strcmp(opcode,"SUBI") == 0)
    {
    if (strcmp(opcode,"ADDI") == 0)
      code[inst_count].op=ADDI;
    else
      code[inst_count].op=SUBI;
    code[inst_count].rd=NOT_USED;
    strcpy(label_fields[inst_count],"");
    ParseRegister(oper1,&(code[inst_count].rt));
    ParseRegister(oper2,&(code[inst_count].rs));
    ParseImmediate(oper3,&(code[inst_count].imm));
    }
  else if (strcmp(opcode,"ADD") == 0  ||  strcmp(opcode,"SUB") == 0)
    {
    if (strcmp(opcode,"ADD") == 0)
      code[inst_count].op=ADD;
    else
      code[inst_count].op=SUB;
    code[inst_count].imm=NOT_USED;
    strcpy(label_fields[inst_count],"");
    ParseRegister(oper1,&(code[inst_count].rd));
    ParseRegister(oper2,&(code[inst_count].rs));
    ParseRegister(oper3,&(code[inst_count].rt));
    }
  else if (strcmp(opcode,"BNEZ") == 0  ||  strcmp(opcode,"BEQZ") == 0)
    {
    if (strcmp(opcode,"BNEZ") == 0)
      code[inst_count].op=BNEZ;
    else
      code[inst_count].op=BEQZ;
    code[inst_count].imm=NOT_USED;
    code[inst_count].rt=NOT_USED;
    code[inst_count].rd=NOT_USED;
    if (oper3 != NULL)
      {
      printf("Too many fields for the following program line:\n");
      printf("%s\n",input);
      exit(0);
      }
    ParseRegister(oper1,&(code[inst_count].rs));
    strcpy(label_fields[inst_count],oper2);
    }
  else if (strcmp(opcode,"J") == 0)
    {
    code[inst_count].op=J;
    code[inst_count].imm=NOT_USED;
    code[inst_count].rs=NOT_USED;
    code[inst_count].rt=NOT_USED;
    code[inst_count].rd=NOT_USED;
    if (oper3 != NULL  ||  oper2 != NULL)
      {
      printf("Too many fields for the following program line:\n");
      printf("%s\n",input);
      exit(0);
      }
    strcpy(label_fields[inst_count],oper1);
    }
  else if (strcmp(opcode,"LW") == 0)
    {
    code[inst_count].op=LW;
    code[inst_count].imm=NOT_USED;
    strcpy(label_fields[inst_count],"");
    code[inst_count].rd=NOT_USED;
    if (oper3 != NULL)
      {
      printf("Too many fields for the following program line:\n");
      printf("%s\n",input);
      exit(0);
      }
    ParseRegister(oper1,&(code[inst_count].rt));
    ParseAddress(oper2,&(code[inst_count].rs),&(code[inst_count].imm));
    }
  else if (strcmp(opcode,"SW") == 0)
    {
    code[inst_count].op=SW;
    code[inst_count].imm=NOT_USED;
    strcpy(label_fields[inst_count],"");
    code[inst_count].rd=NOT_USED;
    if (oper3 != NULL)
      {
      printf("Too many fields for the following program line:\n");
      printf("%s\n",input);
      exit(0);
      }
    ParseAddress(oper1,&(code[inst_count].rs),&(code[inst_count].imm));
    ParseRegister(oper2,&(code[inst_count].rt));
    }
  else
    {
    printf("Unrecognized op-code (%s) on the following line:\n",opcode);
    printf("%s",input);
    exit(0);
    }
  for (i=0; i<inst_count; i++)
    if (label[0] != '\0'  &&  strcmp(code_labels[i],label) == 0)
      break;
  if (i < inst_count)
    {
    printf("Duplicate label on the following line:\n");
    printf("%s\n",input);
    exit(0);
    }
  strcpy(code_labels[inst_count],label);
  inst_count++;
  }
fclose(fpt);

	/* 2nd pass assemble converts labels to address values */
if (DEBUG_ASSEMBLER)
  printf("SECOND PASS\n");
for (i=0; i<inst_count; i++)
  {
  if (strcmp(label_fields[i],"") != 0)
    {
    for (j=0; j<inst_count; j++)
      if (strcmp(label_fields[i],code_labels[j]) == 0)
	break;
    if (j == inst_count)
      {
      printf("No program line identified with the following label:\n");
      printf("%s\n",label_fields[i]);
      exit(0);
      }
    code[i].imm=j-i-1;	    /* extra -1 for earlier NPC add+1 */
    }
  if (DEBUG_ASSEMBLER)
    printf("%d OPCODE %d   OP1 %d   OP2 %d   OP3 %d   IMMED %d\n",i,
	code[i].op,code[i].rd,code[i].rs,code[i].rt,code[i].imm);
  }

*code_length=inst_count;
}


	/*
	** Separates a string into tokens.  Assumes zero-three tokens.
	*/

void ParseLineIntoTokens(char *line,		/* input */
                         char *parse_chars,	/* token separators */
                         char **field1,		/* output; may be NULL */
                         char **field2,		/* output; may be NULL */
                         char **field3)		/* output; may be NULL */

{
char	*field4,input[80];

strcpy(input,line);	/* only used for error reporting */
*field1=strtok(line,parse_chars);
if (*field1 != NULL)
  *field2=strtok(NULL,parse_chars);
else
  *field2=NULL;
if (*field2 != NULL)
  *field3=strtok(NULL,parse_chars);
else
  *field3=NULL;
if (*field3 != NULL)
  field4=strtok(NULL,parse_chars);
else
  field4=NULL;
if (field4 != NULL)
  {
  printf("Too many fields in the following line or operand field:\n");
  printf("%s",input);
  exit(0);
  }
}


	/*
	** Parses a register string into the register tag.
	*/

void ParseRegister(char	*operand,	/* input */
                    int *reg_tag)	/* output */

{
if (strcmp(operand,"R0") == 0) *reg_tag=R0;
else if (strcmp(operand,"R1") == 0) *reg_tag=R1;
else if (strcmp(operand,"R2") == 0) *reg_tag=R2;
else if (strcmp(operand,"R3") == 0) *reg_tag=R3;
else if (strcmp(operand,"R4") == 0) *reg_tag=R4;
else if (strcmp(operand,"R5") == 0) *reg_tag=R5;
else if (strcmp(operand,"R6") == 0) *reg_tag=R6;
else if (strcmp(operand,"R7") == 0) *reg_tag=R7;
else if (strcmp(operand,"R8") == 0) *reg_tag=R8;
else if (strcmp(operand,"R9") == 0) *reg_tag=R9;
else if (strcmp(operand,"R10") == 0) *reg_tag=R10;
else if (strcmp(operand,"R11") == 0) *reg_tag=R11;
else if (strcmp(operand,"R12") == 0) *reg_tag=R12;
else if (strcmp(operand,"R13") == 0) *reg_tag=R13;
else if (strcmp(operand,"R14") == 0) *reg_tag=R14;
else if (strcmp(operand,"R15") == 0) *reg_tag=R15;
else
  {
  printf("Unrecognizable register field:\n%s\n",operand);
  exit(0);
  }
}


	/*
	** Parses an immediate string into the immediate value.
	*/

void ParseImmediate(char *operand,	/* input */
                    int *value)		/* output */

{
int	i;

i=-1;
if (operand[0] == '#'  &&  strlen(operand) > 1  &&
	!(operand[1] == '-'  &&  strlen(operand) == 2))
  {
  for (i=1; i<strlen(operand); i++)
    if ((operand[i] < '0'  ||  operand[i] > '9')  &&
	!(i == 1  &&  operand[i] == '-'))
      break;
  *value=atoi(&(operand[1]));
  }
if (i < strlen(operand))
  {
  printf("Unrecognizable immediate field:\n%s\n",operand);
  exit(0);
  }
}


	/*
	** Parses an address (register & offset) into the register
	** tag and immediate value.
	*/

void ParseAddress(char *operand,	/* input */
                   int *reg_tag,	/* output */
                   int *value)		/* output */

{
int	i;
char	reg_string[80],immed_string[80];

i=0;
while (i < strlen(operand)  &&  ((i == 0  &&  operand[i] == '-')  ||
			(operand[i] >= '0'  &&  operand[i] <= '9')))
  i++;
if (i == 0  ||  i == strlen(operand)  ||  (i == 1  &&  operand[0] == '-'))
  {
  printf("Unrecognizable address field:\n%s\n",operand);
  exit(0);
  }
strcpy(reg_string,&(operand[i]));
strcpy(immed_string,operand);
immed_string[i]='\0';
*value=atoi(immed_string);
if (reg_string[0] != '('  ||  reg_string[strlen(reg_string)-1] != ')')
  {
  printf("Unrecognizable address field:\n%s\n",operand);
  exit(0);
  }
reg_string[strlen(reg_string)-1]='\0';
ParseRegister(&(reg_string[1]),reg_tag);
}




