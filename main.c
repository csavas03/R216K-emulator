/*
	R216K[2/4/8/64]A EMULATOR with GTK+
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <gtk/gtk.h>

GtkWidget *main_window;
GtkBuilder *builder;

GtkSpinButton *speed_spin_button;
GtkEntry *entry_reg_rx[16];
GtkEntry *entry_reg_writemask;
GtkEntry *entry_reg_valonstack;
GtkLabel *open_file_status;
GtkProgressBar *fast_mode_progress_bar;
GtkTextView *memory_dis_asm_text;
GtkEntry *mem_value_s_entryx[4];
GtkEntry *mem_addr_entryx[4];
GtkLabel *mem_disassembly_label;
GtkLabel *mem_view_label[2];
int mem_addr_vals[4] = {0x0000,0x0001,0x0002,0x0003};//16
int mem_addr_watch[2] = {0x0000,0x0008};//16
GtkLabel *flag_labels[4];//CARR - OVF - ZERO - SIGN
GtkLabel *r2term_input_buffer_label;
GtkEntry *fast_amount_entry;

char *last_filename = NULL;
int speed_button_val = 60;//16
int slowModeState = 0;//bool
int pauseatHLT = 1;//bool
int pauseatIP = 0;//bool
int pauseatIPaddr = 0;//16
int pauseatIO = 0;//bool
int R2TERM_PORT = 0;//8
int R2TERM_PORT_IN = 0;//8

uint16_t INPUT_BUFFER[256];
int INPUT_BUFFER_SIZE = 0;//16
int INPUT_BUFFER_P = 0;//16
int bump_rec = 0xFFFF;//16
int Tcursor = 0;//8

const int INST_CLASS[] = {3,3,3,3,3,3,3,3,2,3,3,3,3,3,3,3,0,2,3,3,3,3,3,3,1,1,3,3,2,1,2,0};//0='0' 1='1' 2='1*' 3='2' <-- 8bit

int R2_SIZE = 2048-1;//DEFAULT 2K (bitmask)

uint32_t R2_MEM[65536];
uint16_t R2_REG[16];//14=SP 15=IP // 16
int R2_WM = 0;//Write Mask (SWM)
int R2_SHIFT_PREV = 0;
int F_Carr = 0;//bool
int F_Over = 0;//bool
int F_Zero = 0;//bool
int F_Sign = 0;//bool

int O1 = 0;int O1T = 0;//0=REG 1=MEM 2=CONST //16/8
int O2 = 0;int O2T = 0;//0=REG 1=MEM 2=CONST //16/8

void updateIObufferlabel(){
	char tmp[22];
	snprintf(tmp,22,"INPUT P/S: %03d/%03d",INPUT_BUFFER_P,INPUT_BUFFER_SIZE);
	gtk_label_set_text(r2term_input_buffer_label,tmp);
}
void toR2TERM(int d){//16
	if(d & 0x1000){
		Tcursor = d & 0xFF;
		return;
	}
	if(d & 0x2000){//0x20BF
		char color_arr[] = {30,34,32,36,31,35,33,37,90,94,92,96,91,95,93,97};//To match colors in xterm. //8
		printf("\033[%d;%dm",color_arr[d & 0xF],color_arr[(d>>4) & 0xF]+10);
		return;
	}
	if(d<10) d+=0x30;	///OPTIONAL - Some of my R2 programs use 0x0 - 0x9 as '0' - '9'
	if(d==10) d=0x30;	///OPTIONAL - or 0xA as '0'

	if(d == 0x7F){//0x7F is a full block on the R2TERM - used as blinking cursor.
		printf("\033[%d;%dH\u2588",(Tcursor>>4)+3,(Tcursor&0xF)+2);//UNICODE for FULL BLOCK
	} else {
		printf("\033[%d;%dH%c",(Tcursor>>4)+3,(Tcursor&0xF)+2,d & 0xFF);
	}
	fflush(stdout);

	Tcursor++;
	if(Tcursor>=16*13) Tcursor = 0;
}
void init_reset_r2term(){
	Tcursor = 0;
	//system("tput civis;clear");
	puts("\033[?25l"			//tput civis
		 "\033[3J\033[H\033[2J"	//clear
		 "\033[97;40m\033[1;1H"	//color + cursor pos
		 "     <R2TERM>     \n"
		 "/----------------\\\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "|                |\n"
		 "\\----------------/");
	fflush(stdout);
}

void Roper(int class, int inst){//8-32 //Updates O[1/2] and O[1/2]T
	int type = (inst >> 20) & 0xF;//8
	int subt = (inst >> 15) & 0x1;//8

	int qR1,qR2,qRB,qI1,qI2,qR2F,qI2F;//8-8-8-16-16-8-16
	switch(class){
		case 0://0
			break;
		case 1://1
			qR1 = inst & 0xF;
			qRB = (inst >> 16) & 0xF;
			qI1 = (inst >> 4) & 0xFFFF;
			O1T = 1;
			switch(type){
				case 0x0:
					O1 = qR1;
					O1T = 0;
					break;
				case 0x4:
					O1 = R2_REG[qR1];
					break;
				case 0xC:
					if(subt) O1 = R2_REG[qRB] - R2_REG[qR1];
					else O1 = R2_REG[qRB] + R2_REG[qR1];
					break;
				case 0x5:
					O1 = qI1;
					break;
				case 0xD:
					if(subt) O1 = R2_REG[qRB] - (qI1 & 0x7FF);
					else O1 = R2_REG[qRB] + (qI1 & 0x7FF);
					break;
			}
			break;
		case 2://1*
			qR2 = (inst >> 4) & 0xF;
			qRB = (inst >> 16) & 0xF;
			qI2 = (inst >> 4) & 0xFFFF;
			O1T = 1;
			switch(type){
				case 0x0:
					O1 = qR2;
					O1T = 0;
					break;
				case 0x1:
					O1 = R2_REG[qR2];
					break;
				case 0x9:
					if(subt) O1 = R2_REG[qRB] - R2_REG[qR2];
					else O1 = R2_REG[qRB] + R2_REG[qR2];
					break;
				case 0x2:
					O1 = qI2;O1T = 2;
					break;
				case 0x3:
					O1 = qI2;
					break;
				case 0xB:
					if(subt) O1 = R2_REG[qRB] - (qI2 & 0x7FF);
					else O1 = R2_REG[qRB] + (qI2 & 0x7FF);
					break;
			}
			break;
		case 3://2
			qR1 = inst & 0xF;
			qR2 = (inst >> 4) & 0xF;//CHANGING
			qRB = (inst >> 16) & 0xF;
			qI1 = (inst >> 4) & 0xFFFF;
			qI2 = (inst >> 4) & 0xFFFF;//CHANGING
			qR2F = inst & 0xF;//CHANGING
			qI2F = inst & 0xFFFF;//CHANGING
			switch(type){
				case 0x0:
					O1 = qR1;O1T = 0;
					O2 = qR2;O2T = 0;
					break;
				case 0x1:
					O1 = qR1;O1T = 0;
					O2 = R2_REG[qR2];O2T = 1;
					break;
				case 0x9:
					O1 = qR1;O1T = 0;
					if(subt) O2 = R2_REG[qRB] - R2_REG[qR2];
					else O2 = R2_REG[qRB] + R2_REG[qR2];
					O2T = 1;
					break;
				case 0x2:
					O1 = qR1;O1T = 0;
					O2 = qI1;O2T = 2;
					break;
				case 0x3:
					O1 = qR1;O1T = 0;
					O2 = qI1;O2T = 1;
					break;
				case 0xB:
					O1 = qR1;O1T = 0;
					if(subt) O2 = R2_REG[qRB] - (qI1 & 0x7FF);
					else O2 = R2_REG[qRB] + (qI1 & 0x7FF);
					O2T = 1;
					break;
				case 0x4:
					O1 = R2_REG[qR1];O1T = 1;
					O2 = qR2;O2T = 0;
					break;
				case 0xC:
					O2 = qR2;O2T = 0;
					if(subt) O1 = R2_REG[qRB] - R2_REG[qR1];
					else O1 = R2_REG[qRB] + R2_REG[qR1];
					O1T = 1;
					break;
				case 0x5:
					O1 = qI1;O1T = 1;
					O2 = qR2F;O2T = 0;
					break;
				case 0xD:
					O2 = qR2F;O2T = 0;
					if(subt) O1 = R2_REG[qRB] - (qI1 & 0x7FF);
					else O1 = R2_REG[qRB] + (qI1 & 0x7FF);
					O1T = 1;
					break;
				case 0x6:
					O1 = R2_REG[qR1];O1T = 1;
					O2 = qI1;O2T = 2;
					break;
				case 0xE:
					O2 = (qI2 & 0x7FF);O2T = 2;
					if(subt) O1 = R2_REG[qRB] - R2_REG[qR1];
					else O1 = R2_REG[qRB] + R2_REG[qR1];
					O1T = 1;
					break;
				case 0x7:
					O1 = qI1;O1T = 1;
					O2 = (qI2F & 0xF);O2T = 2;
					break;
				case 0xF:
					O2 = (qI2F & 0xF);O2T = 2;
					if(subt) O1 = R2_REG[qRB] - (qI1 & 0x7FF);
					else O1 = R2_REG[qRB] + (qI1 & 0x7FF);
					O1T = 1;
					break;
			}
			break;
	}
}
int RrOP1(){//16 //Read value of op 1
	switch(O1T){
		case 0://REG
			return R2_REG[O1];
		case 1://MEM
			return R2_MEM[O1 & R2_SIZE] & 0xFFFF;
		case 2://CONST
			return O1;
		default:
			fprintf(stderr,"Error at RrOP1.\n");
	}
	return 0;
}
int RrOP2(){//16 //Read value of op 2
	switch(O2T){
		case 0://REG
			return R2_REG[O2];
		case 1://MEM
			return R2_MEM[O2 & R2_SIZE] & 0xFFFF;
		case 2://CONST
			return O2;
		default:
			fprintf(stderr,"Error at RrOP2.\n");
	}
	return 0;
}
void RwOP1(int val){//16 //Write to op 1
	switch(O1T){
		case 0://REG
			R2_REG[O1] = val;
			break;
		case 1://MEM
			R2_MEM[O1 & R2_SIZE] = val | R2_WM | 0x20000000;
			break;
		case 2://CONST
		default:
			fprintf(stderr,"Error at RwOP1.\n");
	}
}
/*void RwOP2(int val){//16 //UNUSED
	switch(O2T){
		case 0://REG
			R2_REG[O2] = val;
			break;
		case 1://MEM
			R2_MEM[O2 & R2_SIZE] = val | R2_WM | 0x20000000;
			break;
		case 2://CONST
		default:
			fprintf(stderr,"Error at RwOP2.\n");
	}
}*/
void R_flag(int val){//16
	F_Zero = (val == 0);
	F_Sign = ((val & 0x8000) != 0);
}
void Rstep(){//Executes one instruction.
	int saveIP = R2_REG[15];//16
	int inst = R2_MEM[saveIP & R2_SIZE];
	int comm = (inst >> 24) & 0x1F;//8
	int class= INST_CLASS[comm];
	Roper(class,inst);
	int q1 = RrOP1();//16
	int q2 = RrOP2();//16/4
	int q3;//16/32
	int WriteBack = (comm >> 3) ^ 0x01;
	switch(comm){
		case 0x00://MOV
			RwOP1(q2);R_flag(q2);F_Carr = 0;F_Over = 0;
			break;
		case 0x01://AND
		case 0x09://ANDS
			q3 = q1 & q2;
			if(WriteBack) RwOP1(q3);
			R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x02://OR
		case 0x0A://ORS
			q3 = q1 | q2;
			if(WriteBack) RwOP1(q3);
			R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x03://XOR
		case 0x0B://XORS
			q3 = q1 ^ q2;
			if(WriteBack) RwOP1(q3);
			R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x04://ADD
		case 0x0C://ADDS
			q3 = q1 + q2;
			if(WriteBack) RwOP1(q3 & 0xFFFF);
			R_flag(q3 & 0xFFFF);F_Carr = q3 >> 16;F_Over = ((q1>>15) == (q2>>15)) && (((q3>>15)&1) != (q2>>15));
			break;
		case 0x05://ADC
		case 0x0D://ADCS
			q3 = q1 + q2 + F_Carr;
			if(WriteBack) RwOP1(q3 & 0xFFFF);
			R_flag(q3 & 0xFFFF);F_Carr = q3 >> 16;F_Over = ((q1>>15) == (q2>>15)) && (((q3>>15)&1) != (q2>>15));
			break;
		case 0x06://SUB
		case 0x0E://SUBS / CMP
			q3 = (q1 - q2) & 0xFFFF;
			if(WriteBack) RwOP1(q3);
			R_flag(q3);F_Carr = (q1 < q2);F_Over = ((q1>>15)!=(q2>>15)) && ((q2>>15)==(q3>>15));
			break;
		case 0x07://SBB
		case 0x0F://SBBS
			q3 = (q1 - q2 - F_Carr) & 0xFFFF;
			if(WriteBack) RwOP1(q3);
			R_flag(q3);F_Carr = (q1 < q2);F_Over = ((q1>>15)!=(q2>>15)) && ((q2>>15)==(q3>>15));
			break;
		case 0x08://SWM
			R_flag(q1);F_Carr = 0;F_Over = 0;
			R2_WM = (q1 & 0x1FFF) << 16;
			break;
		case 0x10://HLT
			if(pauseatHLT) slowModeState = 0;
			break;
		case 0x11://J**
			switch(inst & 0xF){
				case 0x0://JMP			TRUE
					R2_REG[15] = q1;
					break;
				case 0x1://JN			FALSE	NOP
					break;
				case 0x2://JB/JNAE/JC	C = 1
					if(F_Carr) R2_REG[15] = q1;
					break;
				case 0x3://JNB/JAE/JNC	C = 0
					if(F_Carr == 0) R2_REG[15] = q1;
					break;
				case 0x4://JO			O = 1
					if(F_Over) R2_REG[15] = q1;
					break;
				case 0x5://JNO			O = 0
					if(F_Over == 0) R2_REG[15] = q1;
					break;
				case 0x6://JS			S = 1
					if(F_Sign) R2_REG[15] = q1;
					break;
				case 0x7://JNS			S = 0
					if(F_Sign == 0) R2_REG[15] = q1;
					break;
				case 0x8://JE/JZ		Z = 1
					if(F_Zero) R2_REG[15] = q1;
					break;
				case 0x9://JNE/JNZ		Z = 0
					if(F_Zero == 0) R2_REG[15] = q1;
					break;
				case 0xA://JLE/JNG		Z = 1 OR S != O
					if(F_Zero || F_Sign!=F_Over) R2_REG[15] = q1;
					break;
				case 0xB://JNE/JNZ		Z = 0 OR S = O
					if(F_Zero == 0 || F_Sign==F_Over) R2_REG[15] = q1;
					break;
				case 0xC://JL/JNGE		S != O
					if(F_Sign != F_Over) R2_REG[15] = q1;
					break;
				case 0xD://JNL/JGE		S = O
					if(F_Sign == F_Over) R2_REG[15] = q1;
					break;
				case 0xE://JBE/JNA		C = 1 OR Z = 1
					if(F_Carr || F_Zero) R2_REG[15] = q1;
					break;
				case 0xF://JNBE/JA		C = 0 AND Z = 0
					if(F_Carr == 0 && F_Zero == 0) R2_REG[15] = q1;
					break;
			}
			break;
		case 0x12://ROL <<
			q2 &= 0xF;
			q3 = q1;
			for(int i=0;i<q2;i++) q3 = ((q3 << 1) | (q3 >> 15)) & 0xFFFF;
			RwOP1(q3);R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x13://ROR >>
			q2 &= 0xF;
			q3 = q1;
			for(int i=0;i<q2;i++) q3 = ((q3 & 0x01)<<15) | (q3 >> 1);
			RwOP1(q3);R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x14://SHL <<
			q2 &= 0xF;
			q3 = (q1 << q2) & 0xFFFF;
			RwOP1(q3);R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x15://SHR >>
			q2 &= 0xF;
			q3 = q1 >> q2;
			RwOP1(q3);R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		/*case 0x16://SCL << (OLD)
			q1 = RrOP1();
			q2 = RrOP2() & 0xF;
			qb = q1;
			for(int i=0;i<q2;i++) qb = ((qb << 1) | ((R2_SHIFT_PREV >> (15-i)) & 0x01)) & 0xFFFF;
			RwOP1(qb);R_flag(qb);F_Carr = 0;F_Over = 0;
			break;*/
		case 0x16://SCL << (NEW)
			q2 &= 0xF;
			q3 = ((((q1<<16) | R2_SHIFT_PREV) << q2) >> 16) & 0xFFFF;
			RwOP1(q3);R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x17://SCR	>>
			q2 &= 0xF;
			q3 = (((R2_SHIFT_PREV << 16) | q1) >> q2) & 0xFFFF;
			RwOP1(q3);R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x18://BUMP
			break;
		case 0x19://WAIT
			if(INPUT_BUFFER_P < INPUT_BUFFER_SIZE) bump_rec = R2TERM_PORT_IN;
			RwOP1(bump_rec);
			R_flag(bump_rec);F_Carr = 0;F_Over = 0;
			bump_rec = 0xFFFF;
			break;
		case 0x1A://SEND
			if(q1 == R2TERM_PORT) toR2TERM(q2);
			if(pauseatIO) slowModeState = 0;
			break;
		case 0x1B://RECV
			if(pauseatIO) slowModeState = 0;
			if(q2 == R2TERM_PORT_IN){
				if(INPUT_BUFFER_P < INPUT_BUFFER_SIZE){
					F_Carr = 1;
					int r_val = INPUT_BUFFER[INPUT_BUFFER_P++];//16
					RwOP1(r_val & 0xFFFF);
					F_Zero = (r_val == 0);
					F_Sign = ((r_val & 0x8000) != 0);
					if(INPUT_BUFFER_P == INPUT_BUFFER_SIZE){
						INPUT_BUFFER_SIZE = 0;
						INPUT_BUFFER_P = 0;
					}
					updateIObufferlabel();
				} else {
					F_Carr = 0;
				}
			}
			break;
		case 0x1C://PUSH
			R2_MEM[(--R2_REG[14]) & R2_SIZE] = q1 | R2_WM | 0x20000000;
			R_flag(q1);F_Carr = 0;F_Over = 0;
			break;
		case 0x1D://POP
			q3 = R2_MEM[(R2_REG[14]++) & R2_SIZE];
			RwOP1(q3);
			R_flag(q3);F_Carr = 0;F_Over = 0;
			break;
		case 0x1E://CALL
			R2_MEM[(--R2_REG[14]) & R2_SIZE] = (R2_REG[15] + 1) | R2_WM | 0x20000000;
			R2_REG[15] = q1;
			break;
		case 0x1F://RET
			R2_REG[15] = R2_MEM[(R2_REG[14]++) & R2_SIZE] & 0xFFFF;
			break;
		default:
			fprintf(stderr,"Error at Rstep.\n");
	}
	if(class != 0) R2_SHIFT_PREV = q1;
	if(saveIP == R2_REG[15]) R2_REG[15]++;
}
int disassemble(int inst,char *trg){
	int inst_t = (inst >> 24) & 0x1F;
	int inst_c = INST_CLASS[(inst >> 24) & 0x1F];
	int oper_t = (inst >> 20) & 0xF;
	char *instName[] = {
		"MOV ","AND ","OR  ","XOR ","ADD ","ADC ","SUB ","SBB ",
		"SWM ","ANDS","ORS ","XORS","ADDS","ADCS","CMP ","SBBS",
		"HLT ","    ","ROL ","ROR ","SHL ","SHR ", "SCL ","SCR ",
		"BUMP","WAIT","SEND","RECV","PUSH","POP ","CALL","RET "
	};
	char *jmpName[16] = {
		"JMP ","JN  ","JB  ","JNB ","JO  ","JNO ","JS  ","JNS ",
		"JE  ","JNE ","JNG ","JG  ","JL  ","JNL ","JBE ","JA  "
	};
	char addsub = (inst & 0x8000)?'-':'+';
	char *inst_n;
	if(inst_t != 0x11) inst_n = instName[inst_t];
	else inst_n = jmpName[inst & 0x0F];
	if(inst_c == 0){//0
		return sprintf(trg,"%s\n",inst_n);
	} else if(inst_c == 1){//1
		int c1_r1 = inst & 0x0F;
		int c1_rb = (inst >> 16) & 0x0F;
		int c1_i1 = (inst >> 4) & 0xFFFF;//16/11 bit
		switch(oper_t){
			case 0x00:
				return sprintf(trg,"%s\tr%d\n",inst_n,c1_r1);
			case 0x04:
				return sprintf(trg,"%s\t[r%d]\n",inst_n,c1_r1);
			case 0x0C:
				return sprintf(trg,"%s\t[r%d%cr%d]\n",inst_n,c1_rb,addsub,c1_r1);
			case 0x05:
				return sprintf(trg,"%s\t[0x%X]\n",inst_n,c1_i1);
			case 0x0D:
				return sprintf(trg,"%s\t[r%d%c0x%X]\n",inst_n,c1_rb,addsub,c1_i1&0x7FF);
		}
	} else if(inst_c == 2){//1*
		int c2_r2 = (inst >> 4) & 0x0F;
		int c2_rb = (inst >> 16) & 0x0F;
		int c2_i1 = (inst >> 4) & 0xFFFF;//16/11 bit
		switch(oper_t){
			case 0x00:
				return sprintf(trg,"%s\tr%d\n",inst_n,c2_r2);
			case 0x01:
				return sprintf(trg,"%s\t[r%d]\n",inst_n,c2_r2);
			case 0x09:
				return sprintf(trg,"%s\t[r%d%cr%d]\n",inst_n,c2_rb,addsub,c2_r2);
			case 0x02:
				return sprintf(trg,"%s\t0x%X\n",inst_n,c2_i1);
			case 0x03:
				return sprintf(trg,"%s\t[0x%X]\n",inst_n,c2_i1);
			case 0x0B:
				return sprintf(trg,"%s\t[r%d%c0x%X]\n",inst_n,c2_rb,addsub,c2_i1&0x7FF);
		}
	} else /*if(inst_c == 3)*/{//2
		int c3_r1 = inst & 0x0F;
		int c3_r2_4 = (inst >> 4) & 0x0F;
		int c3_r2_0 = inst & 0x0F;
		int c3_rb = (inst >> 16) & 0x0F;
		int c3_i1 = (inst >> 4) & 0xFFFF;//16/11 bit
		int c3_i2_11b = (inst >> 4) & 0x7FF;
		int c3_i2_4b = inst & 0x0F;
		switch(oper_t){
			case 0x00:
				return sprintf(trg,"%s\tr%d, r%d\n",inst_n,c3_r1,c3_r2_4);
			case 0x01:
				return sprintf(trg,"%s\tr%d, [r%d]\n",inst_n,c3_r1,c3_r2_4);
			case 0x09:
				return sprintf(trg,"%s\tr%d, [r%d%cr%d]\n",inst_n,c3_r1,c3_rb,addsub,c3_r2_4);
			case 0x02:
				return sprintf(trg,"%s\tr%d, 0x%X\n",inst_n,c3_r1,c3_i1);
			case 0x03:
				return sprintf(trg,"%s\tr%d, [0x%X]\n",inst_n,c3_r1,c3_i1);
			case 0x0B:
				return sprintf(trg,"%s\tr%d, [r%d%c0x%X]\n",inst_n,c3_r1,c3_rb,addsub,c3_i1&0x7FF);
			case 0x04:
				return sprintf(trg,"%s\t[r%d], r%d\n",inst_n,c3_r1,c3_r2_4);
			case 0x0C:
				return sprintf(trg,"%s\t[r%d%cr%d], r%d\n",inst_n,c3_rb,addsub,c3_r1,c3_r2_4);
			case 0x05:
				return sprintf(trg,"%s\t[0x%X], r%d\n",inst_n,c3_i1,c3_i1);
			case 0x0D:
				return sprintf(trg,"%s\t[r%d%c0x%X], r%d\n",inst_n,c3_rb,addsub,c3_i1&0x7FF,c3_r2_0);
			case 0x06:
				return sprintf(trg,"%s\t[r%d], 0x%X\n",inst_n,c3_r1,c3_i1);
			case 0x0E:
				return sprintf(trg,"%s\t[r%d%cr%d], 0x%X\n",inst_n,c3_rb,addsub,c3_r1,c3_i2_11b);
			case 0x07:
				return sprintf(trg,"%s\t[0x%X], 0x%X\n",inst_n,c3_i1,c3_i2_4b);
			case 0x0F:
				return sprintf(trg,"%s\t[r%d%c0x%X], 0x%X\n",inst_n,c3_rb,addsub,c3_i1&0x7FF,c3_i2_4b);
		}
	}
	return sprintf(trg,"<I>\t%04X\n",inst & 0xFFFF);
}
void updateDisassembly(){
	int p = 0;
	char tmp[32*12];
	int ip = R2_REG[15];
	static int qwe = 0;// <-- STATIC

	if(qwe+12<=ip || qwe>=ip){
		qwe =  ip;
	}
	if(qwe<0) qwe = 0;

	for(int i=0;i<12;i++){
		p += sprintf(&tmp[p],"%04X %s ",qwe+i,((qwe+i == ip)?"---->":"     "));
		p += disassemble(R2_MEM[qwe+i],&tmp[p]);//<-^-- NULL terminator gets overwritten
	}
	gtk_label_set_text(mem_disassembly_label,tmp);
}
int hexStringToInt(const char *p,int maxlen){//32-8
	int l=0;
	while(p[l]) l++;
	if(l>maxlen) return -1;
	for(int i=0;i<l;i++){
		int q = p[i];
		if(!((q>='0' && q<='9') || (q>='A' && q<='F') || (q>='a' && q<='f'))){
			return -1;
		}
	}
	return (int) strtol(p,0,16);
}
void updateMemWatch(){
	char tmp[232];//(28C+1NL)*8
	for(int q=0;q<2;q++){
		int p = 0;
		for(int i=0;i<8;i++){
			int w = mem_addr_watch[q];//16
			int v = R2_MEM[w+i];//32
			p += sprintf(&tmp[p],"0x%04X   0x%08X - %d\n",w+i,v,v&0xFFFF);
		}
		gtk_label_set_text(mem_view_label[q],tmp);
	}
}
void updateMemEditor(){
	for(int i=0;i<4;i++){
		char q[9];
		snprintf(q,9,"%08X",R2_MEM[mem_addr_vals[i]]);
		gtk_entry_set_text(mem_value_s_entryx[i],q);
	}
}
void updateRegWatch(){
	char tmp[5];
	for(int i=0;i<16;i++){
		snprintf(tmp,5,"%04X",R2_REG[i]);
		gtk_entry_set_text(entry_reg_rx[i],tmp);
	}
	snprintf(tmp,5,"%04X",R2_WM >> 16);
	gtk_entry_set_text(entry_reg_writemask,tmp);
	snprintf(tmp,5,"%04X",R2_MEM[R2_REG[14] & R2_SIZE] & 0xFFFF);
	gtk_entry_set_text(entry_reg_valonstack,tmp);
}
void updateFlagWatch(){
	gtk_label_set_text(flag_labels[0],(F_Carr)?"CARRY":"");
	gtk_label_set_text(flag_labels[1],(F_Over)?"OVERFLOW":"");
	gtk_label_set_text(flag_labels[2],(F_Zero)?"ZERO":"");
	gtk_label_set_text(flag_labels[3],(F_Sign)?"SIGN":"");
}
gboolean slowSTEPmode(){
	if(!slowModeState) return 0;
	Rstep();
	updateRegWatch();
	updateMemEditor();
	updateMemWatch();
	updateFlagWatch();
	updateDisassembly();
	if(pauseatIP && ((R2_REG[15] & R2_SIZE) == pauseatIPaddr)) slowModeState = 0;
	return slowModeState;
}
void on_reg_modified(GtkEntry *e){
	int q = hexStringToInt(gtk_entry_get_text(e),4);//32
	if(q == -1){
		gtk_entry_set_text(e,"----");
		return;
	}
	char tmp[5];snprintf(tmp,5,"%04X",q);gtk_entry_set_text(e,tmp);
	const char *p = gtk_entry_get_placeholder_text(e);
	switch(p[0]){
		case 'R': ;
			int regN = atoi(&p[1]) & 0xF;
			R2_REG[regN] = q;
			if(regN == 15) updateDisassembly();
			break;
		case 'W':
			R2_WM = ((q & 0x1FFF) << 16);
			break;
		case 'S':
			R2_MEM[R2_REG[14] & R2_SIZE] = q | R2_WM | 0x20000000;
			break;
		default:
			fprintf(stderr,"Error at on_reg_modified.\n");
	}
}
void resetAll(){
	Tcursor = 0;
	for(int i=0;i<16;i++) R2_REG[i]=0;
	F_Carr=0;F_Over=0;F_Zero=0;F_Sign=0;
	INPUT_BUFFER_SIZE = 0;
	INPUT_BUFFER_P = 0;
	updateMemEditor();
	updateMemWatch();
	updateDisassembly();
	updateRegWatch();
	updateFlagWatch();
	updateIObufferlabel();
	init_reset_r2term();
}
void on_open_r2_bin_button_clicked(GtkButton *b){
	if(b){
		GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",GTK_WINDOW(main_window),GTK_FILE_CHOOSER_ACTION_OPEN,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,NULL);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT){
			last_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		} else {
			gtk_widget_destroy(dialog);
			return;
		}
		gtk_widget_destroy (dialog);
		int p=0;while(last_filename[p]) p++;while(last_filename[p]!='/') p--;///Is is guaranteed to contain '/' ?
		gtk_button_set_label(b,&last_filename[p+1]);
	}

	FILE *file = fopen(last_filename,"r");
	if(!file) {
		char *ERR1 = "Can't access file.";
		gtk_label_set_text(open_file_status,ERR1);fputs(ERR1,stderr);
		return;
	}
	fseek(file, 0L, SEEK_END);
	int fsize = ftell(file);
	if(fsize & 0x3){
		char *ERR2 = "FILE SIZE NOT MULTIPLE OF 4.";
		gtk_label_set_text(open_file_status,ERR2);fputs(ERR2,stderr);
		fclose(file);
		return;
	}
	if(fsize > 4*64*1024){
		char *ERR3 = "LARGER THAN (4*)64 KiB.";
		gtk_label_set_text(open_file_status,ERR3);fputs(ERR3,stderr);
		fclose(file);
		return;
	}
	if(fsize > 4*(R2_SIZE+1)){
		char *ERR4 = "FILE IS LARGER THAN THE SELECTED MODELS MEMORY.";
		gtk_label_set_text(open_file_status,ERR4);fputs(ERR4,stderr);
		fclose(file);
		return;
	}
	rewind(file);
	int ws = fread(R2_MEM,4,65536,file);//Assuming little endian for both.
	fclose(file);
	for(int i=ws;i<65536;i++){//Clear the rest of the memory.
		R2_MEM[i]=0x20000000;
	}
	for(int i=0;i<ws;i++){
		if(R2_MEM[i] > 0x3FFFFFFF || (R2_MEM[i] & 0x20000000)==0){
			char *ERR5 = "SOME WORDS WERE OUT OF RANGE/NO MSB SET.";
			gtk_label_set_text(open_file_status,ERR5);fputs(ERR5,stderr);
			for(int i=0;i<65536;i++) R2_MEM[i] = 0x20000000;
			return;
		}
	}
	gtk_label_set_text(open_file_status,"FILE SUCCESFULLY LOADED");
	resetAll();
}
void on_model_select_combobox_changed(GtkComboBox *widget){
	int q = atoi(gtk_combo_box_get_active_id(widget));//8
	if(q!=2 && q!=4 && q!=8 && q!=64) fprintf(stderr,"Combobox invalid value.\n");
	R2_SIZE = (q<<10) - 1;
}
void on_speed_button_value_changed(GtkSpinButton *s){
	speed_button_val = gtk_spin_button_get_value_as_int(s);
}
void on_start_button_clicked(){
	if(slowModeState == 0){
		slowModeState = 1;
		g_timeout_add(1000 / speed_button_val,slowSTEPmode,NULL);
	}
}
void on_pause_button_clicked(){
	slowModeState = 0;
}
void on_step_button_clicked(){
	Rstep();
	updateRegWatch();
	updateMemEditor();
	updateMemWatch();
	updateFlagWatch();
	updateDisassembly();
}
void on_reset_button_clicked(){
	slowModeState = 0;
	if(last_filename) on_open_r2_bin_button_clicked(NULL);
	else for(int i=0;i<65536;i++) R2_MEM[i] = 0x20000000;
	resetAll();
}
void on_fast_button_clicked(GtkButton *b){
	if(slowModeState) return;
	slowModeState = 1;
	long long int am = strtoll(gtk_entry_get_text(fast_amount_entry),0,0);
	if(am < 1) return;
	for(long long int i=0;i<am && slowModeState;i++){
		Rstep();
		if(pauseatIP && ((R2_REG[15] & R2_SIZE) == pauseatIPaddr)) slowModeState = 0;
	}
	slowModeState = 0;
	updateRegWatch();
	updateMemEditor();
	updateMemWatch();
	updateFlagWatch();
	updateDisassembly();
}
void on_pause_at_io_tbutton_toggled(GtkToggleButton *t){
	pauseatIO = gtk_toggle_button_get_active(t);
}
void on_pause_at_ip_toggled(GtkToggleButton *t){
	pauseatIP = gtk_toggle_button_get_active(t);
}
void on_ignore_hlt_tbutton_toggled(GtkToggleButton *t){
	pauseatHLT = !gtk_toggle_button_get_active(t);// <-- INVERTED
}
void on_save_r2_bin_button_clicked(GtkButton *b){
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save File",GTK_WINDOW(main_window),GTK_FILE_CHOOSER_ACTION_SAVE,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),TRUE);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),"Untitled file");
	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT){
		gtk_widget_destroy(dialog);
		return;
	}
	char *fileout = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);

	int p=0;while(fileout[p]) p++;while(fileout[p]!='/') p--;
	gtk_button_set_label(b,&fileout[p+1]);

	FILE *file = fopen(fileout,"w");
	fwrite(R2_MEM,4,R2_SIZE+1,file);
	fclose(file);
	g_free(fileout);
}
void updateMemAddrVals(){
	for(int i=0;i<4;i++){
		int q = hexStringToInt(gtk_entry_get_text(mem_addr_entryx[i]),4);//32
		if(q == -1 || q > R2_SIZE){
			gtk_entry_set_text(mem_addr_entryx[i],"----");
			return;
		}
		mem_addr_vals[i] = q;
	}
}
void on_mem_addr_entry_activate(GtkEntry *e){
	updateMemAddrVals();
	int id = (gtk_entry_get_placeholder_text(e)[3]-'0') & 0x03;
	int addrMem = mem_addr_vals[id];//16
	if(addrMem <= R2_SIZE+1){
		char q[9];
		snprintf(q,9,"%08X",R2_MEM[addrMem]);
		gtk_entry_set_text(mem_value_s_entryx[id],q);
	} else {
		gtk_entry_set_text(mem_value_s_entryx[id],"N/A  OOB");
	}
}
void on_mem_value_s_entry_activate(GtkEntry *e){
	int q = hexStringToInt(gtk_entry_get_text(e),8);//32
	if(q == -1 || q > 0x3FFFFFFF){
		gtk_entry_set_text(e,"--------");
		return;
	}
	int addr = mem_addr_vals[gtk_entry_get_placeholder_text(e)[3]-'0'];
	R2_MEM[addr] = (q | 0x20000000);
	updateMemWatch();
	updateDisassembly();
}
void on_mem_view_addr0_activate(GtkEntry *e){
	int q = hexStringToInt(gtk_entry_get_text(e),4);//32
	if(q == -1) gtk_entry_set_text(e,"----");
	mem_addr_watch[0]=q;
	updateMemWatch();
}
void on_mem_view_addr1_activate(GtkEntry *e){
	int q = hexStringToInt(gtk_entry_get_text(e),4);//32
	if(q == -1) gtk_entry_set_text(e,"----");
	mem_addr_watch[1]=q;
	updateMemWatch();
}
void on_save_disassembly_button_clicked(GtkButton *b){
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save File",GTK_WINDOW(main_window),GTK_FILE_CHOOSER_ACTION_SAVE,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),TRUE);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),"Untitled file");
	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT){
		gtk_widget_destroy(dialog);
		return;
	}
	char *fileout = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);

	int p=0;while(fileout[p]) p++;while(fileout[p]!='/') p--;
	gtk_button_set_label(b,&fileout[p+1]);

	FILE *f = fopen(fileout, "w");
	if(f == NULL){
		fprintf(stderr,"Can't open disassembly file!\n");
		return;
	}
	int q = 0;
	char *tmp = malloc(32*(R2_SIZE+1));
	for(int i=0;i<=R2_SIZE;i++){
		q += sprintf(&tmp[q],"%04X\t",i);
		q += disassemble(R2_MEM[i],&tmp[q]);
	}
	fputs(tmp,f);
	fclose(f);
	g_free(fileout);
}
void on_pause_at_ip_addr_entry_activate(GtkEntry *e){
	int q = hexStringToInt(gtk_entry_get_text(e),4);//32
	if(q == -1){
		gtk_entry_set_text(e,"----");
		return;
	}
	pauseatIPaddr = q;
	char tmp[5];snprintf(tmp,5,"%04X",q);
	gtk_entry_set_text(e,tmp);
}
void on_clear_screen_button_clicked(){
	init_reset_r2term();
}
void on_r2term_port_entry_activate(GtkEntry *e){
	const char *p = gtk_entry_get_text(e);
	for(int i=0;p[i];i++){
		int q = p[i];
		if(q<'0' || q>'9'){
			gtk_entry_set_text(e,"---");
			return;
		}
	}
	int port = strtol(p,0,10);
	if(port>=256){
		gtk_entry_set_text(e,"---");
		return;
	}
	R2TERM_PORT = port;
}
void on_r2term_port_in_entry_activate(GtkEntry *e){
	const char *p = gtk_entry_get_text(e);
	for(int i=0;p[i];i++){
		int q = p[i];
		if(q<'0' || q>'9'){
			gtk_entry_set_text(e,"---");
			return;
		}
	}
	int port = strtol(p,0,10);
	if(port>=256){
		gtk_entry_set_text(e,"---");
		return;
	}
	R2TERM_PORT_IN = port;
}
void on_r2term_ascii_text_entry_activate(GtkEntry *e){
	const char *q = gtk_entry_get_text(e);
	for(int i=0;q[i] && INPUT_BUFFER_SIZE<256;i++){
		INPUT_BUFFER[INPUT_BUFFER_SIZE++] = q[i] & 0xFF;
	}
	gtk_entry_set_text(e,"");
	updateIObufferlabel();
}
void on_r2term_hex_text_entry_activate(GtkEntry *e){
	const char *ep = gtk_entry_get_text(e);
	int i = 0;char *q;//16
	for(;;){
		INPUT_BUFFER[INPUT_BUFFER_SIZE++] = strtol(&ep[i],&q,0) & 0xFFFF;
		i = q - ep + 1;
		if(INPUT_BUFFER_SIZE>255 || ep[i-1]==0x00) break;
	}
	gtk_entry_set_text(e,"");
	updateIObufferlabel();
}
void on_r2term_keys_text_entry_activate(GtkEntry *e){
	if(INPUT_BUFFER_SIZE == 256) return;
	const char *q = gtk_entry_get_text(e);
	char w = q[0];if(w>='a' && w<='z') w -= 0x20;//make uppercase
	if(w == 'E') INPUT_BUFFER[INPUT_BUFFER_SIZE++] = '\r';//ENTER
	else if(w == 'B') INPUT_BUFFER[INPUT_BUFFER_SIZE++] = '\b';//BACKSPACE
	else if(w == 'T') INPUT_BUFFER[INPUT_BUFFER_SIZE++] = '\t';//TAB
	else if(w == 'S') INPUT_BUFFER[INPUT_BUFFER_SIZE++] = ' ';//SPACE
	else if(w == '0' || w == 'N') INPUT_BUFFER[INPUT_BUFFER_SIZE++] = 0x00;//NULL
	else {
		gtk_entry_set_text(e,"INVALID CHARACTER");
		return;
	}
	gtk_entry_set_text(e,"");
	updateIObufferlabel();
}
void on_newline_r2term_button_clicked(){
	if(INPUT_BUFFER_SIZE == 256) return;
	INPUT_BUFFER[INPUT_BUFFER_SIZE++] = '\r';
	updateIObufferlabel();
}

void getPointers(){
	char arr[19];
	for(int i=0;i<16;i++){snprintf(arr,14,"entry_reg_r%d",i);entry_reg_rx[i]=GTK_ENTRY(gtk_builder_get_object(builder,arr));}
	speed_spin_button = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,"speed_spin_button"));
	entry_reg_writemask = GTK_ENTRY(gtk_builder_get_object(builder,"entry_reg_writemask"));
	entry_reg_valonstack = GTK_ENTRY(gtk_builder_get_object(builder,"entry_reg_valonstack"));
	open_file_status = GTK_LABEL(gtk_builder_get_object(builder,"open_file_status"));
	fast_mode_progress_bar = GTK_PROGRESS_BAR(gtk_builder_get_object(builder,"fast_mode_progress_bar"));
	mem_disassembly_label = GTK_LABEL(gtk_builder_get_object(builder,"mem_disassembly_label"));
	mem_view_label[0] = GTK_LABEL(gtk_builder_get_object(builder,"mem_view_label0"));
	mem_view_label[1] = GTK_LABEL(gtk_builder_get_object(builder,"mem_view_label1"));
	gtk_misc_set_alignment(GTK_MISC(mem_disassembly_label), 0.0f, 0.0f);
	gtk_misc_set_alignment(GTK_MISC(mem_view_label[0]), 0.0f, 0.0f);
	gtk_misc_set_alignment(GTK_MISC(mem_view_label[1]), 0.0f, 0.0f);
	for(int i=0;i<4;i++){snprintf(arr,19,"mem_value_s_entry%d",i);mem_value_s_entryx[i]=GTK_ENTRY(gtk_builder_get_object(builder,arr));}
	for(int i=0;i<4;i++){snprintf(arr,16,"mem_addr_entry%d",i);mem_addr_entryx[i]=GTK_ENTRY(gtk_builder_get_object(builder,arr));}
	flag_labels[0] = GTK_LABEL(gtk_builder_get_object(builder,"flag_carry_label"));
	flag_labels[1] = GTK_LABEL(gtk_builder_get_object(builder,"flag_overflow_label"));
	flag_labels[2] = GTK_LABEL(gtk_builder_get_object(builder,"flag_zero_label"));
	flag_labels[3] = GTK_LABEL(gtk_builder_get_object(builder,"flag_sign_label"));
	r2term_input_buffer_label = GTK_LABEL(gtk_builder_get_object(builder,"r2term_input_buffer_label"));
	fast_amount_entry = GTK_ENTRY(gtk_builder_get_object(builder,"fast_amount_entry"));
}
int main(int argc, char *argv[]){
	gtk_init(&argc, &argv);

	builder = gtk_builder_new_from_file("glade.glade");

	main_window = GTK_WIDGET(gtk_builder_get_object(builder,"main_window"));
	g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit),NULL);
	gtk_builder_connect_signals(builder,NULL);

	getPointers();
	for(int i=0;i<65536;i++) R2_MEM[i] = 0x20000000;
	updateMemWatch();
	updateDisassembly();
	init_reset_r2term();

	gtk_widget_show(main_window);

	/*if(argc == 2){
		last_filename = argv[1];
		on_open_r2_bin_button_clicked(NULL);
	}*/

	gtk_main();
	return EXIT_SUCCESS;
}
