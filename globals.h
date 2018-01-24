#define ADDI	100
#define ADD	101
#define SUBI	102
#define SUB	103
#define LW	104
#define SW	105
#define BEQZ	106
#define BNEZ	107
#define J	108

#define R0	0
#define R1	1
#define R2	2
#define R3	3
#define R4	4
#define R5	5
#define R6	6
#define R7	7
#define R8	8
#define R9	9
#define R10	10
#define R11	11
#define R12	12
#define R13	13
#define R14	14
#define R15	15

	/*
	** The structure for an assembled instruction.  An array of
	** these structures makes up the "machine code".  Depending
	** upon the type of instruction, most of the fields may not
	** be needed; the exception is the inst field, which must
	** always be defined.
	*/

struct instruction {
	int	op;		/* operation (op-code) */
	int	rd;		/* rd register tag */
	int	rs;		/* rs register tag */
	int	rt;		/* rt register tag */
	int	imm;		/* actual immediate value in assembler inst */
};

#define MAX_LINES_OF_CODE	100
#define NOT_USED		-1
#define MAX_WORDS_OF_DATA	1000

	/* function prototypes */
void AssembleSimpleDLX(char *,struct instruction *,int *);
void ParseLineIntoTokens(char *,char *,char **,char **,char **);
void ParseRegister(char *,int *);
void ParseImmediate(char *,int *);
void ParseAddress(char *,int *,int *);
