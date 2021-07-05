#include <bits/stdc++.h>
using namespace std;

#ifndef RISC_V_UNNAME_HPP
#define RISC_V_UNNAME_HPP

unsigned int pc;//无符号
unsigned int mem[0x20000];//2*16^4
unsigned int reg[32];//寄存器
stringstream sstream;
unsigned int upup[32];
int jump_pre,predict,pre_right,pre_wrong;//[jump_pre] 0/1:not jump ; 2/3:jump
bool ifpre=false,over=false;
enum TYPE{LUI=0,AUIPC,JAL,JALR,
    BEQ,BNE,BLT,BGE,BLTU,BGEU,
    LB,LH,LW,LBU,LHU,
    SB,SH,SW,
    ADDI,SLTI,SLTIU,XORI,ORI,ANDI,
    SLLI,SRLI,SRAI,
    ADD,SUB,SLL,SLT,SLTU,XOR,SRL,SRA,OR,AND,
    WRONG};
string ss[55] = {"LUI", "AUIPC", "JAL", "JALR",
                 "BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU",
                 "LB", "LH", "LW", "LBU", "LHU", "SB", "SH", "SW",
                 "ADDI", "SLTI", "SLTIU", "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI",
                 "ADD", "SUB", "SLL", "SLT", "SLTU", "XOR", "SRL", "SRA", "OR", "AND",
                 "WRONG"};
struct pipeline_reg {
    TYPE op_type;
    unsigned int pre_code, tmp_pc, adr;//tmppc:当前pc   adr：address（read from
    int rd, rs1, rs2, imm, shamt,predictor,funct3,funct7;
    unsigned int rd_val, rs1_val, rs2_val;
    pipeline_reg() {
        op_type = WRONG;
        rd = rs1 = rs2 = imm = predictor = funct3 = funct7 = 0;
        pre_code = rd_val = rs1_val = rs2_val = 0;
    }
}IF_ID,ID_EX,EX_MEM,MEM_WB;
bool IFing=1,IDing=0,EXing=0,MEMing=0,WBing=0;
int MEM_time,MEM_rd,EX_rd;
unsigned int MEM_rd_val,EX_rd_val;

void get_imm_U() {
    ID_EX.rd = (ID_EX.pre_code >> 7) & 31;
    ID_EX.imm = ID_EX.pre_code & upup[12];
}
void get_imm_J() {
    ID_EX.rd = (ID_EX.pre_code >> 7) & 31;
    ID_EX.imm = (((ID_EX.pre_code >> 20) & 1) << 11) | (((ID_EX.pre_code >> 12) & 255) << 12) |
                (((ID_EX.pre_code >> 21) & 1023) << 1);
    if (ID_EX.pre_code >> 31)ID_EX.imm |= upup[20];//前补位
}
void get_imm_I() {
    ID_EX.rd = (ID_EX.pre_code >> 7) & 31;
    ID_EX.funct3 = (ID_EX.pre_code >> 12) & 7;
    ID_EX.rs1 = (ID_EX.pre_code >> 15) & 31;
    ID_EX.imm = ID_EX.pre_code >> 20;
    if (ID_EX.pre_code >> 31)ID_EX.imm |= upup[12];
}
void get_imm_I_() {
    ID_EX.rd = (ID_EX.pre_code >> 7) & 31;
    ID_EX.funct3 = (ID_EX.pre_code >> 12) & 7;
    ID_EX.funct7 = (ID_EX.pre_code >> 25) & 127;
    ID_EX.rs1 = (ID_EX.pre_code >> 15) & 31;
    ID_EX.shamt = (ID_EX.pre_code >> 20) & 63;
}
void get_imm_B() {
    ID_EX.funct3 = (ID_EX.pre_code >> 12) & 7;
    ID_EX.rs1 = (ID_EX.pre_code >> 15) & 31;
    ID_EX.rs2 = (ID_EX.pre_code >> 20) & 31;
    ID_EX.imm = (((ID_EX.pre_code >> 8) & 15) << 1) | (((ID_EX.pre_code >> 25) & 127) << 5) |
                (((ID_EX.pre_code >> 7) & 1) << 11);
    if (ID_EX.pre_code >> 31)ID_EX.imm |= upup[12];
}
void get_imm_S() {
    ID_EX.funct3 = (ID_EX.pre_code >> 12) & 7;
    ID_EX.rs1 = (ID_EX.pre_code >> 15) & 31;
    ID_EX.rs2 = (ID_EX.pre_code >> 20) & 31;
    ID_EX.imm = ((ID_EX.pre_code >> 7) & 31) | (((ID_EX.pre_code >> 25) & 127)<<5);
    if (ID_EX.pre_code >> 31)ID_EX.imm |= upup[12];
}
void get_imm_R() {
    ID_EX.rd = (IF_ID.pre_code >> 7) & 31;
    ID_EX.funct3 = (IF_ID.pre_code >> 12) & 7;
    ID_EX.funct7 = (IF_ID.pre_code >> 25) & 127;
    ID_EX.rs1 = (IF_ID.pre_code >> 15) & 31;
    ID_EX.rs2 = (IF_ID.pre_code >> 20) & 31;
}

void READin() {
    for (int i = 0; i < 32; ++i)upup[i] = -1 << i;
    string str;
    while (cin >> str) {
        if (str[0] == '@') {
            sstream << str.substr(1, 8);
            sstream >> hex >> uppercase >> pc;//hex -> dec (uppercase 16进制采用大写字母
            sstream.clear();
        } else {
            sstream << str;
            sstream >> hex >> uppercase >> mem[pc++];//hex -> dec
            sstream.clear();
        }
    }
}

void Instruction_Fetch() {
    if (IDing||(over)) return;
//     puts("-----IF-Begin-----");
    IF_ID = pipeline_reg();
    unsigned int ini_code;
    ini_code = (mem[pc]) | (mem[pc + 1] << 8) | (mem[pc + 2] << 16) | (mem[pc + 3] << 24) ;
    IF_ID.pre_code = ini_code, IF_ID.tmp_pc = pc;
    if(IF_ID.pre_code==0xFF00513){over=1;return ;}
    pc += 4;
    IFing = 0;
    IDing = 1;
//    puts("-----IF-End-----");
}

void Instruction_Decode(){//读reg
    if(EXing||(!IDing)) return ;
//    puts("-----ID-Begin-----");
    ID_EX=IF_ID;

    int opcode,funct3,funct7;
    opcode = ID_EX.pre_code & 127;
    switch (opcode) {
        case 55:
            ID_EX.op_type = LUI;

            get_imm_U();
            break;
        case 23:
            ID_EX.op_type = AUIPC;
            get_imm_U();
            break;
        case 111:
            ID_EX.op_type = JAL;
            get_imm_J();
            break;
        case 103:
            ID_EX.op_type = JALR;
            get_imm_I();
            break;
        case 99: {
            get_imm_B();
            funct3 = (ID_EX.pre_code >> 12) & 7;
            if (funct3 == 0)ID_EX.op_type = BEQ;
            else if (funct3 == 1)ID_EX.op_type = BNE;
            else if (funct3 == 4)ID_EX.op_type = BLT;
            else if (funct3 == 5)ID_EX.op_type = BGE;
            else if (funct3 == 6)ID_EX.op_type = BLTU;
            else if (funct3 == 7)ID_EX.op_type = BGEU;
            break;
        }
        case 3: {
            get_imm_I();
            funct3 = (ID_EX.pre_code >> 12) & 7;
            if(funct3==0)ID_EX.op_type = LB;
            else if(funct3==1)ID_EX.op_type = LH;
            else if(funct3==2)ID_EX.op_type = LW;
            else if(funct3==4)ID_EX.op_type = LBU;
            else if(funct3==5)ID_EX.op_type = LHU;
            break;
        }
        case 35: {
            get_imm_S();
            funct3 = (ID_EX.pre_code >> 12) & 7;
            if(funct3==0)ID_EX.op_type = SB;
            else if(funct3==1)ID_EX.op_type = SH;
            else if(funct3==2)ID_EX.op_type = SW;
            break;
        }
        case 19: {
            funct3 = (ID_EX.pre_code >> 12) & 7;
            if(funct3==0)ID_EX.op_type=ADDI,get_imm_I();
            else if(funct3==2)ID_EX.op_type=SLTI,get_imm_I();
            else if(funct3==3)ID_EX.op_type=SLTIU,get_imm_I();
            else if(funct3==4)ID_EX.op_type=XORI,get_imm_I();
            else if(funct3==6)ID_EX.op_type=ORI,get_imm_I();
            else if(funct3==7)ID_EX.op_type=ANDI,get_imm_I();
            else if(funct3==1){
               funct7=(ID_EX.pre_code>>25)&127;
               if(funct7==0)ID_EX.op_type=SLLI,get_imm_I_();
            }
            else if(funct3==5){
                funct7=(ID_EX.pre_code>>25)&127;
                if(funct7==0)ID_EX.op_type=SRLI,get_imm_I_();
                else if(funct7==32)ID_EX.op_type=SRAI,get_imm_I_();
            }
            break;
        }
        case 51: {
            get_imm_R();
            funct3 = (ID_EX.pre_code >> 12) & 7;
            if (funct3 == 0) {
                funct7 = (ID_EX.pre_code >> 25) & 127;
                if (funct7 == 0)ID_EX.op_type = ADD;
                else if (funct7 == 32)ID_EX.op_type = SUB;
            } else if (funct3 == 1)ID_EX.op_type = SLL;
            else if (funct3 == 2)ID_EX.op_type = SLT;
            else if (funct3 == 3)ID_EX.op_type = SLTU;
            else if (funct3 == 4)ID_EX.op_type = XOR;
            else if (funct3 == 5) {
                funct7 = (ID_EX.pre_code >> 25) & 127;
                if (funct7 == 0)ID_EX.op_type = SRL;
                else if (funct7 == 32)ID_EX.op_type = SRA;
            } else if (funct3 == 6)ID_EX.op_type = OR;
            else if (funct3 == 7)ID_EX.op_type = AND;
            break;
        }
        default : ID_EX.op_type=WRONG;break;
    }
//    printf("ID_before read reg :: %s  pc = %08x  rs1_reg[%d] = %d  rs2_reg[%d] = %d  rd_reg[%d] = %d\n", ss[ID_EX.op_type].c_str(), ID_EX.tmp_pc, ID_EX.rs1, reg[ID_EX.rs1], ID_EX.rs2, reg[ID_EX.rs2], ID_EX.rd, ID_EX.rd_val);

//    cout<<ID_EX.rs2<<' '<<ID_EX.rs1<<MEM_WB.rd<<endl;

    if(ID_EX.rs1&&ID_EX.rs1==MEM_rd)ID_EX.rs1_val=MEM_rd_val;
    else if(ID_EX.rs1&&ID_EX.rs1==EX_rd)ID_EX.rs1_val=EX_rd_val;
    else ID_EX.rs1_val=reg[ID_EX.rs1];
    if(ID_EX.rs2&&ID_EX.rs2==MEM_rd)ID_EX.rs2_val=MEM_rd_val;
    else if(ID_EX.rs2&&ID_EX.rs2==EX_rd)ID_EX.rs2_val=EX_rd_val;
    else ID_EX.rs2_val=reg[ID_EX.rs2];
    MEM_rd=0,EX_rd=0;

//    printf("ID_after read reg :: %s  pc = %08x  rs1_reg[%d] = %d  rs2_reg[%d] = %d  rd_reg[%d] = %d\n", ss[ID_EX.op_type].c_str(), ID_EX.tmp_pc, ID_EX.rs1, ID_EX.rs1_val, ID_EX.rs2, ID_EX.rs2_val, ID_EX.rd, ID_EX.rd_val);

    switch(ID_EX.op_type){
        case JAL: pc = ID_EX.tmp_pc + ID_EX.imm; break;
        case JALR: pc = (ID_EX.imm + ID_EX.rs1_val) & upup[1]; break;
        case BEQ: case BNE: case BLT: case BGE: case BLTU: case BGEU:
            if(jump_pre>1)ifpre=1,pc = ID_EX.tmp_pc+ ID_EX.imm;
            else if(jump_pre<2)ifpre=0;
            break;
        default : break;
    }
    IDing=0,EXing=1;
// puts("-----ID-End-----");
}

bool jump_judge(pipeline_reg x){
    if(x.op_type==BEQ&&x.rs1_val==x.rs2_val)return 1;
    else if(x.op_type==BNE&&x.rs1_val!=x.rs2_val)return 1;
    else if(x.op_type==BLT&&((signed)x.rs1_val)<((signed)x.rs2_val))return 1;
    else if(x.op_type==BGE&&((signed)x.rs1_val)>=((signed)x.rs2_val))return 1;
    else if(x.op_type==BLTU&&x.rs1_val<x.rs2_val)return 1;//(unsigned
    else if(x.op_type==BGEU&&x.rs1_val >=x.rs2_val)return 1;//(unsigned
    else return 0;
}
void Execute(){
    if(MEMing||(!EXing)) return ;
// puts("-----EX-Begin-----");
    EX_MEM=ID_EX;
    switch (EX_MEM.op_type) {
        case LUI:EX_MEM.rd_val=EX_MEM.imm;break;
        case AUIPC:EX_MEM.rd_val=EX_MEM.tmp_pc+EX_MEM.imm;break;
        //jump start
        case JAL:EX_MEM.rd_val=EX_MEM.tmp_pc+4;break;
        case JALR:EX_MEM.rd_val=EX_MEM.tmp_pc+4;break;
        //condition_jump start
        case BEQ:case BNE:case BLT:case BGE:case BLTU:case BGEU:
            if(jump_judge(EX_MEM)){//jump
                jump_pre=min(jump_pre+1,3);
                if(ifpre)pre_right++;
                else {
                    pre_wrong++;
                    pc = EX_MEM.tmp_pc + EX_MEM.imm, IDing = 0;
                    break;
                }
            }else {//not jump
                jump_pre = max(jump_pre - 1, 0);
                if (!ifpre)pre_right++;
                else {
                    pre_wrong++;
                    pc = EX_MEM.tmp_pc + 4, IDing = 0;
                    break;
                }
            }
        //jump over
        case LB: case LH: case LW: case LBU: case LHU: case SB: case SH: case SW:EX_MEM.adr=EX_MEM.rs1_val+EX_MEM.imm;break;
        case ADDI:EX_MEM.rd_val=EX_MEM.rs1_val+EX_MEM.imm;break;
        case SLTI:((signed)EX_MEM.rs1_val)<((signed)EX_MEM.imm) ? EX_MEM.rd_val= 1 : 0;break;
        case SLTIU:EX_MEM.rs1_val<EX_MEM.imm ? EX_MEM.rd_val= 1 : 0;break;
        case XORI:EX_MEM.rd_val=EX_MEM.rs1_val ^ EX_MEM.imm;break;
        case ORI:EX_MEM.rd_val=EX_MEM.rs1_val | EX_MEM.imm;break;
        case ANDI:EX_MEM.rd_val=EX_MEM.rs1_val & EX_MEM.imm;break;
        case SLLI:EX_MEM.rd_val=EX_MEM.rs1_val<<EX_MEM.shamt;break;
        case SRLI:EX_MEM.rd_val=EX_MEM.rs1_val>>EX_MEM.shamt;break;
        case SRAI:EX_MEM.rd_val=((signed)EX_MEM.rs1_val)>>EX_MEM.shamt;break;
        case ADD:EX_MEM.rd_val=EX_MEM.rs1_val+EX_MEM.rs2_val;break;
        case SUB:EX_MEM.rd_val=EX_MEM.rs1_val-EX_MEM.rs2_val;break;
        case SLL:EX_MEM.rd_val=EX_MEM.rs1_val<<(EX_MEM.rs2_val&31);break;
        case SLT:((signed)EX_MEM.rs1_val)<((signed)EX_MEM.rs2_val) ? EX_MEM.rd_val=1:0;break;
        case SLTU:EX_MEM.rs1_val<EX_MEM.rs2_val ? EX_MEM.rd_val=1:0;break;
        case XOR:EX_MEM.rd_val=EX_MEM.rs1_val ^ EX_MEM.rs2_val;break;
        case SRL:EX_MEM.rd_val=EX_MEM.rs1_val>>(EX_MEM.rs2_val&31);break;
        case SRA:EX_MEM.rd_val=((signed)EX_MEM.rs1_val)>>(EX_MEM.rs2_val&31);break;
        case OR:EX_MEM.rd_val=EX_MEM.rs1_val ^ EX_MEM.rs2_val;break;
        case AND:EX_MEM.rd_val=EX_MEM.rs1_val & EX_MEM.rs2_val;break;
        default: break;
    }
//    printf("EX_:: %s  pc = %08x  rs1_reg[%d] = %d  rs2_reg[%d] = %d  rd_reg[%d] = %d\n", ss[EX_MEM.op_type].c_str(), EX_MEM.tmp_pc, EX_MEM.rs1, EX_MEM.rs1_val, EX_MEM.rs2, EX_MEM.rs2_val, EX_MEM.rd, EX_MEM.rd_val);


    //forward：
    //如果decode获取rs1/2_val，且rs1/2为该时刻内存读写操作的未知rd，则改正rs1/2_val为前步骤修改后内存的内存值
    if(ID_EX.rs1&&ID_EX.rs1==EX_MEM.rd)ID_EX.rs1_val=EX_MEM.rd_val;
    if(ID_EX.rs2&&ID_EX.rs2==EX_MEM.rd)ID_EX.rs2_val=EX_MEM.rd_val;

    MEM_time=0;//能进行EX即MEM_time已持续至3

    //EX_:用于改下个任务的rs
    EX_rd=EX_MEM.rd,EX_rd_val=EX_MEM.rd_val;

    EXing=0,MEMing=1;
// puts("-----EX-End-----");
}

// 倒序  √
// forwarding数据前传  √
// MEM三周期  √
// 分支预测  √

void Memory(){//改/用reg，真改mem
    if(WBing||(!MEMing)) return ;
// puts("-----MEM-Begin-----");
    MEM_WB=EX_MEM;
    switch (MEM_WB.op_type) {
        case LB:case LH:case LW:case LBU:case LHU:case SB:case SH:case SW:{
            if(MEM_time<2){MEM_time++;return ;}//需要MEM但未达到3周期，不修改MEMing -> EXing、IDing、IFing不进行
            switch (MEM_WB.op_type) {
                case LB:MEM_WB.rd_val=(char)mem[MEM_WB.adr];break;
                case LH:MEM_WB.rd_val=(short)mem[MEM_WB.adr]|mem[MEM_WB.adr+1]<<8;break;//short >=16bits
                case LW:MEM_WB.rd_val=mem[MEM_WB.adr]|mem[MEM_WB.adr+1]<<8|mem[MEM_WB.adr+2]<<16|mem[MEM_WB.adr+3]<<24;break;
                case LBU:MEM_WB.rd_val=mem[MEM_WB.adr];break;
                case LHU:MEM_WB.rd_val=mem[MEM_WB.adr]|mem[MEM_WB.adr+1]<<8;break;
                case SB:mem[MEM_WB.adr]=MEM_WB.rs2_val&255;break;
                case SH:mem[MEM_WB.adr]=MEM_WB.rs2_val&255,mem[MEM_WB.adr+1]=(MEM_WB.rs2_val>>8)&255;break;
                case SW:mem[MEM_WB.adr]=MEM_WB.rs2_val&255,mem[MEM_WB.adr+1]=(MEM_WB.rs2_val>>8)&255,
                            mem[MEM_WB.adr+2]=(MEM_WB.rs2_val>>16)&255,mem[MEM_WB.adr+3]=(MEM_WB.rs2_val>>24)&255;break;
                default: break;
            }
        }
    }
//forward：
    //如果decode获取rs1/2_val并即将进入execute(EXing=1)，且rs1/2为该时刻内存读写操作的未知rd，则改正rs1/2_val为前步骤修改后内存的内存值
    if(EXing) {
        if (EXing && ID_EX.rs1 && ID_EX.rs1 == MEM_WB.rd)ID_EX.rs1_val = MEM_WB.rd_val;
        if (EXing && ID_EX.rs2 && ID_EX.rs2 == MEM_WB.rd)ID_EX.rs2_val = MEM_WB.rd_val;
    }
//MEM_:用于改下下个任务的rs
    MEM_rd=MEM_WB.rd,MEM_rd_val=MEM_WB.rd_val;
    MEMing=0,WBing=1;
// puts("-----MEM-End-----");
}

void WriteBack(){
    if(!WBing)return ;
// puts("-----WB-Begin-----");
    switch (MEM_WB.op_type) {
        case LUI: case AUIPC: case JAL: case JALR: case LB:case LH: case LW: case LBU: case LHU:case ADDI: case SLTI: case SLTIU: case XORI: case ORI: case ANDI: case SLLI: case SRLI: case SRAI:case ADD: case SUB: case SLL: case SLT: case SLTU: case XOR: case SRL: case SRA: case OR: case AND:
            if(MEM_WB.rd)reg[MEM_WB.rd] = MEM_WB.rd_val;break;
        default:break;
    }
    WBing=0;
// puts("-----WB-End-----");
}

void RUN(){
    pc=0;
    int no=0;
    while((WBing||MEMing||EXing||IDing||IFing)){
//        cout<<"--------------No. no---------------"<<++no<<endl;
        WriteBack();
        Memory();
        Execute();
        Instruction_Decode();
        Instruction_Fetch();
    }
    cout<<(reg[10]&255u)<<endl;
}

#endif //RISC_V_UNNAME_HPP
