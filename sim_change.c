#include <stdio.h>
#include <string.h>
#include "globals.h"

// Max cycles simulator will execute -- to stop a runaway simulator
#define FAIL_SAFE_LIMIT  500000

struct	instruction   inst_mem[MAX_LINES_OF_CODE];  // instruction memory
struct 	instruction   IR;                       // instruction register 
int   	data_mem[MAX_WORDS_OF_DATA];             // data memory
int   	int_regs[16];                            // integer register file
int   	PC,NPC;                                  // PC and next PC
int   	A,B;                                     // ID register read values
int   	mem_addr;                                // data memory address
int   	cond;                                    // branch condition test
int   	LMD;                                     // data memory output
int   	ALU_input_1,ALU_input_2,ALU_output;        // ALU intputs and output
int   	wrote_r0=0;                              // register R0 written?
int   	code_length;                             // lines of code in inst mem
int   	cycle;                                   // simulation cycle count
int   	inst_executed;                          // number of instr executed
int   	stall_lw;
int	 	stall_branch;


//design the piplined register
struct pipline {
	struct 	instruction   IR;
	int 	PC;
	int 	NPC;
	int   	A,B; 
	int   	LMD;
	int   	ALU_output;
};

struct pipline IF_ID;
struct pipline ID_EX;
struct pipline EX_MEM;
struct pipline MEM_WB;
struct pipline deposit_IF_ID;
struct pipline deposit_ID_EX;
struct pipline deposit_EX_MEM;
struct pipline deposit_MEM_WB;


void Simulate_pipline_DLX_cycle(){
	if (PC < 0 || PC > code_length){
    printf("Exception: out-of-bounds inst memory access at PC=%d\n",PC);
    exit(0);}
	
	if(stall_lw==1){
	stall_lw=0;	

	
	MEM_stage();
	WB_stage();
	
	
	deposit_EX_MEM.PC= -1;
	deposit_EX_MEM.IR.op=ADDI;
	deposit_EX_MEM.IR.rt=-1;
	deposit_EX_MEM.IR.rs=-1;
	deposit_EX_MEM.IR.rt=-1;
	deposit_EX_MEM.IR.imm=-1;
	
	return(0);}
	
// stall and flushing cause by branch
	if(stall_branch==1){
	deposit_IF_ID.PC=-1;
	deposit_IF_ID.IR.op=ADDI;
	deposit_IF_ID.IR.rt=-1;
	deposit_IF_ID.IR.rs=-1;
	deposit_IF_ID.IR.imm=-1;
	deposit_ID_EX.PC=-1;
	deposit_ID_EX.IR.op=ADDI;
	deposit_ID_EX.IR.rt=-1;
	deposit_ID_EX.IR.rs=-1;
	deposit_ID_EX.IR.imm=-1;
	stall_branch=0;
	}
	
	
	IF_stage();
	ID_stage();
	EX_stage();
	MEM_stage();
	WB_stage();
	
	
}



void WB_stage(){
  /* ------------------------------ WB stage ------------------------------ */
	if ( deposit_MEM_WB.PC < 0||deposit_MEM_WB.PC >= code_length)  { 
		deposit_MEM_WB.A=MEM_WB.A;
		deposit_MEM_WB.B=MEM_WB.B;
		deposit_MEM_WB.NPC=MEM_WB.NPC;
		deposit_MEM_WB.IR=MEM_WB.IR;
		deposit_MEM_WB.PC=MEM_WB.PC;
		deposit_MEM_WB.ALU_output=MEM_WB.ALU_output;
		deposit_MEM_WB.LMD=MEM_WB.LMD;
		return(0);
		}
	
  /* write to register and check if output register is R0 */
	if (deposit_MEM_WB.IR.op == ADD|| deposit_MEM_WB.IR.op == SUB) {
		int_regs[deposit_MEM_WB.IR.rd]=deposit_MEM_WB.ALU_output;
		wrote_r0=(deposit_MEM_WB.IR.rd==R0);
	}
	else if (deposit_MEM_WB.IR.op == ADDI|| deposit_MEM_WB.IR.op == SUBI) {
		int_regs[deposit_MEM_WB.IR.rt]=deposit_MEM_WB.ALU_output;
		wrote_r0=(deposit_MEM_WB.IR.rt==R0);
	} 
	else if (deposit_MEM_WB.IR.op == LW){
		int_regs[deposit_MEM_WB.IR.rt]=deposit_MEM_WB.LMD;
		wrote_r0= (deposit_MEM_WB.IR.rt==R0);
	}
	
	deposit_MEM_WB.A= MEM_WB.A;
	deposit_MEM_WB.B= MEM_WB.B;
	deposit_MEM_WB.NPC =MEM_WB.NPC;
	deposit_MEM_WB.IR =MEM_WB.IR;
    deposit_MEM_WB.PC= MEM_WB.PC;
	deposit_MEM_WB.ALU_output= MEM_WB.ALU_output;
	deposit_MEM_WB.LMD= MEM_WB.LMD;
  

	inst_executed++;

  /* if output register is R0, exit with error */
	if (wrote_r0) {
    printf("Exception: Attempt to overwrite R0 at PC=%d\n",PC);
    exit(0);
  }

}

void MEM_stage() {


  /* ------------------------------ MEM stage ----------------------------- */
  if ( deposit_EX_MEM.PC < 0 ||deposit_EX_MEM.PC >= code_length) { 
	MEM_WB.A= deposit_EX_MEM.A;
	MEM_WB.B =deposit_EX_MEM.B;
	MEM_WB.NPC= deposit_EX_MEM.NPC;
	MEM_WB.IR =deposit_EX_MEM.IR;
    MEM_WB.PC= deposit_EX_MEM.PC;
	MEM_WB.ALU_output= deposit_EX_MEM.ALU_output;
	MEM_WB.LMD =LMD;
	
	deposit_EX_MEM.A =EX_MEM.A;
	deposit_EX_MEM.B =EX_MEM.B;
	deposit_EX_MEM.NPC= EX_MEM.NPC;
	deposit_EX_MEM.IR= EX_MEM.IR;
    deposit_EX_MEM.PC= EX_MEM.PC;
	deposit_EX_MEM.ALU_output =EX_MEM.ALU_output;
    return(0);}
	
  
	
	mem_addr = deposit_EX_MEM.ALU_output;

  /* check if data memory access is within bounds */
	if (deposit_EX_MEM.IR.op ==LW ||deposit_EX_MEM.IR.op == SW){
		if (mem_addr < 0 || mem_addr >= MAX_WORDS_OF_DATA){
		printf("Exception: out-of-bounds data memory access at PC=%d\n",PC);
		exit(0);
		}
	}

	if(deposit_EX_MEM.IR.op == LW)               /* read memory for lw instruction */
		LMD = data_mem[mem_addr];
	else if (deposit_EX_MEM.IR.op== SW )         /* or write to memory for sw instruction */
		data_mem[mem_addr]= deposit_EX_MEM.B;
	
		MEM_WB.A= deposit_EX_MEM.A;
		MEM_WB.B= deposit_EX_MEM.B;
		MEM_WB.NPC =deposit_EX_MEM.NPC;
		MEM_WB.IR =deposit_EX_MEM.IR;
		MEM_WB.PC =deposit_EX_MEM.PC;
		MEM_WB.ALU_output= deposit_EX_MEM.ALU_output;
		MEM_WB.LMD= LMD;
	
		deposit_EX_MEM.A =EX_MEM.A;
		deposit_EX_MEM.B =EX_MEM.B;
		deposit_EX_MEM.NPC= EX_MEM.NPC;
		deposit_EX_MEM.IR= EX_MEM.IR;
		deposit_EX_MEM.PC= EX_MEM.PC;
		deposit_EX_MEM.ALU_output =EX_MEM.ALU_output;
	
	}

	
void EX_stage(){
  /* ------------------------------ EX stage ------------------------------ */
  
  if (deposit_ID_EX.PC < 0||deposit_ID_EX.PC >= code_length)  { 
	EX_MEM.A=A;
	EX_MEM.B=B;
	EX_MEM.NPC= deposit_ID_EX.NPC;
	EX_MEM.IR= deposit_ID_EX.IR;
    EX_MEM.PC =deposit_ID_EX.PC;
	EX_MEM.ALU_output =ALU_output;
	deposit_ID_EX.NPC= ID_EX.NPC;
	deposit_ID_EX.IR= ID_EX.IR;
    deposit_ID_EX.PC= ID_EX.PC;
    return(0);
	}
	
	
	A=int_regs[deposit_ID_EX.IR.rs];   /* read registers */
	
	//DATA fowarding for SW
	if (deposit_ID_EX.IR.op== SW){
	if((deposit_EX_MEM.IR.op == ADD || deposit_EX_MEM.IR.op == SUB )&& deposit_ID_EX.IR.rt==deposit_EX_MEM.IR.rd)
	B= deposit_EX_MEM.ALU_output;
	else if ((deposit_EX_MEM.IR.op == ADDI || deposit_EX_MEM.IR.op == SUBI)&& deposit_ID_EX.IR.rt==deposit_EX_MEM.IR.rt) 
    B=deposit_EX_MEM.ALU_output;
	else if ((deposit_MEM_WB.IR.op == ADD || deposit_MEM_WB.IR.op == SUB )&& deposit_ID_EX.IR.rt==deposit_MEM_WB.IR.rd) 
    B=deposit_MEM_WB.ALU_output;
	else if ((deposit_MEM_WB.IR.op == ADDI || deposit_MEM_WB.IR.op == SUBI) &&deposit_ID_EX.IR.rt==deposit_MEM_WB.IR.rt) 
	B=deposit_MEM_WB.ALU_output;
	else if (deposit_MEM_WB.IR.op == LW && deposit_ID_EX.IR.rt== deposit_MEM_WB.IR.rt) 
	B=deposit_MEM_WB.LMD;}
	else
	B=int_regs[deposit_ID_EX.IR.rt];

	//DATA forwarding for ADD,SUB and so on
	if ((deposit_EX_MEM.IR.op == ADD || deposit_EX_MEM.IR.op ==SUB )&& deposit_ID_EX.IR.rs==deposit_EX_MEM.IR.rd) 
    ALU_input_1=deposit_EX_MEM.ALU_output;
	 else if ((deposit_EX_MEM.IR.op == ADDI || deposit_EX_MEM.IR.op == SUBI) && deposit_ID_EX.IR.rs==deposit_EX_MEM.IR.rt) 
    ALU_input_1=deposit_EX_MEM.ALU_output;
	 else if ((deposit_MEM_WB.IR.op == ADD || deposit_MEM_WB.IR.op == SUB )&& deposit_ID_EX.IR.rs==deposit_MEM_WB.IR.rd) 
    ALU_input_1=deposit_MEM_WB.ALU_output;
	 else if ((deposit_MEM_WB.IR.op == ADDI || deposit_MEM_WB.IR.op == SUBI) && deposit_ID_EX.IR.rs==deposit_MEM_WB.IR.rt) 
    ALU_input_1=deposit_MEM_WB.ALU_output;
	 else if (deposit_MEM_WB.IR.op == LW && deposit_ID_EX.IR.rs==deposit_MEM_WB.IR.rt) 
	ALU_input_1=deposit_MEM_WB.LMD;
	 else
	ALU_input_1 = A;
	
	
	//DATA forwarding for ADD,SUB and so on
	if (deposit_ID_EX.IR.op == ADDI || deposit_ID_EX.IR.op == SUBI || 
		deposit_ID_EX.IR.op == LW   ||deposit_ID_EX.IR.op == SW )
		ALU_input_2 = deposit_ID_EX.IR.imm;
	else if ((deposit_EX_MEM.IR.op == ADD || deposit_EX_MEM.IR.op == SUB )&& deposit_ID_EX.IR.rt==deposit_EX_MEM.IR.rd) 
		ALU_input_2=deposit_EX_MEM.ALU_output;
	else if ((deposit_EX_MEM.IR.op == ADDI || deposit_EX_MEM.IR.op == SUBI)&& deposit_ID_EX.IR.rt==deposit_EX_MEM.IR.rt) 
		ALU_input_2=deposit_EX_MEM.ALU_output;
	else if ((deposit_MEM_WB.IR.op == ADD || deposit_MEM_WB.IR.op == SUB )&&deposit_ID_EX.IR.rt==deposit_MEM_WB.IR.rd) 
		ALU_input_2=deposit_MEM_WB.ALU_output;
	else if ((deposit_MEM_WB.IR.op == ADDI|| deposit_MEM_WB.IR.op == SUBI)&& deposit_ID_EX.IR.rt==deposit_MEM_WB.IR.rt) 
		ALU_input_2=deposit_MEM_WB.ALU_output;
	else if (deposit_MEM_WB.IR.op == LW &&deposit_ID_EX.IR.rt==deposit_MEM_WB.IR.rt) 
		ALU_input_2=deposit_MEM_WB.LMD;
	else
		ALU_input_2 = B;

	/* calculate ALU output */
	if(deposit_ID_EX.IR.op == SUB ||deposit_ID_EX.IR.op == SUBI)
		ALU_output = ALU_input_1 - ALU_input_2;
	else if(deposit_ID_EX.IR.op == ADDI|| deposit_ID_EX.IR.op == ADD || 
		deposit_ID_EX.IR.op == LW || deposit_ID_EX.IR.op == SW )
		ALU_output = ALU_input_1+ ALU_input_2;
	
	
	/* Calculate branch condition codes 
     and change PC if condition is true */
	if (deposit_ID_EX.IR.op==BEQZ)
		cond=(ALU_input_1==0);      //when beqz, true if A is 0
	else if (deposit_ID_EX.IR.op==BNEZ)
		cond=(ALU_input_1!=0);         //when bnez, true if A is not 0
	else if (deposit_ID_EX.IR.op==J)
		cond=1;          //when jump, it is always true
	else
		cond=0;           
		
	if (cond)           
		{
		NPC=deposit_ID_EX.NPC+deposit_ID_EX.IR.imm;
		stall_branch=1;
		}
  
  
	EX_MEM.A=A;
	EX_MEM.B=B;
	EX_MEM.NPC=deposit_ID_EX.NPC;
	EX_MEM.IR=deposit_ID_EX.IR;
    EX_MEM.PC=deposit_ID_EX.PC;
	EX_MEM.ALU_output=ALU_output;
	
	deposit_ID_EX.NPC=ID_EX.NPC;
	deposit_ID_EX.IR=ID_EX.IR;
    deposit_ID_EX.PC=ID_EX.PC;
	
	}

void ID_stage(){

  /* ------------------------------ ID stage ------------------------------ */
   if (deposit_IF_ID.PC < 0||deposit_IF_ID.PC >= code_length)  { 
	ID_EX.NPC=deposit_IF_ID.NPC;
	ID_EX.IR=deposit_IF_ID.IR;
    ID_EX.PC=deposit_IF_ID.PC;
	
	deposit_IF_ID.IR=IF_ID.IR;
	deposit_IF_ID.PC=IF_ID.PC;
	deposit_IF_ID.NPC=IF_ID.NPC;
    return(0);
	}
	
	
	// predict stall or not for lw
	if (deposit_ID_EX.IR.op == LW && (deposit_IF_ID.IR.rt==deposit_ID_EX.IR.rt||deposit_IF_ID.IR.rs==deposit_ID_EX.IR.rt)) //it is forwarding and lw
		stall_lw=1;
  
 
	ID_EX.NPC=deposit_IF_ID.NPC;  
	ID_EX.IR=deposit_IF_ID.IR;
    ID_EX.PC=deposit_IF_ID.PC;
	
	deposit_IF_ID.IR=IF_ID.IR;
	deposit_IF_ID.PC=IF_ID.PC;
	deposit_IF_ID.NPC=IF_ID.NPC;

	
	
	}
	
	void IF_stage(){
	
	if (PC < 0 || PC >= code_length)
	{ IF_ID.PC=PC;
		return(0);}
	
	IR=inst_mem[PC];    /* read instruction memory */
	NPC=PC+1;           /* increment PC */
	IF_ID.IR=IR;
	IF_ID.PC=PC;
	IF_ID.NPC=NPC;

}

	





main(int argc, char**argv){
	int i;

	if(argc!=2) {  /* Check command line inputs */
		printf("Usage: sim [program]\n");
		exit(0);
	}

  /* assemble input program */
	AssembleSimpleDLX(argv[1],inst_mem,&code_length);

  /* set initial simulator values */
	cycle=0;                /* simulator cycle count */
	PC=0;                   /* first instruction to execute from inst mem */
	int_regs[R0]=0;         /* register R0 is alway zero */
	inst_executed=0;
 /* initial the pipline register*/
	IF_ID.PC=-1;
	ID_EX.PC=-1;
	EX_MEM.PC=-1;
	MEM_WB.PC=-1;
	deposit_IF_ID.PC=-1;
	deposit_ID_EX.PC=-1;
	deposit_EX_MEM.PC=-1;
	deposit_MEM_WB.PC=-1;
  
 

  /* Main simulator loop */
  while (deposit_MEM_WB.PC!=code_length){
	
	Simulate_pipline_DLX_cycle();
    PC=NPC;                    /* update PC          */
    cycle+=1;                  /* update cycle count */
	
	

    /* check if simuator is stuck in an infinite loop */
    if (cycle>FAIL_SAFE_LIMIT){    
      printf("\n\n *** Runaway program? (Program halted.) ***\n\n");
      break;
    }
  }


  /* print final register values and simulator statistics */
  printf("Final register file values:\n");
  for (i=0;i<16;i+=4){
    printf("  R%-2d: %-10d  R%-2d: %-10d",i,int_regs[i],i+1,int_regs[i+1]);
    printf("  R%-2d: %-10d  R%-2d: %-10d\n",i+2,int_regs[i+2],i+3,int_regs[i+3]);
  }
  printf("\nCycles executed: %d\n",cycle);
  printf("IPC:  %6.3f\n", (float)inst_executed/(float)cycle);
  printf("CPI:  %6.3f\n", (float)cycle/(float)inst_executed);

}
