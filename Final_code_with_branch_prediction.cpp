#include <stdio.h> 
#include <stdlib.h> 
#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <vector>
#include <cmath>
using namespace std;


class InstMemDec{   //class responsible for
public:             //decoding and splitting
    int op_code[6]; //the instruction
    int readRegister1addr[5];
    int readRegister2addr[5];
    int function[6];
    int immediate[16];
    int rt[5];

    void decode(int instruction[]){
        for(int i=0;i<6;i++)
        {
            op_code[i]=instruction[i];  
            function[i]=instruction[i+26];
//opcode is used to decode what the instruction
//require processor to do. function has the same
//use in the R-type instructions
        }
        for(int i=0;i<5;i++)
        {
            readRegister1addr[i]=instruction[i+6];
            readRegister2addr[i]=instruction[i+11];
            rt[i]=instruction[i+16];
//decoding two read and one write addresses to register
        }
        for(int i=0;i<16;i++)
        {
            immediate[i]=instruction[i+16];
    //value used in immediate addition, AND, OR, ...
        }
    }
};

class Control{  //class control consists of all
public:         //control signals
    bool RegWrite;
    bool ALUSrc;
    bool MemWrite=false;
    int ALUoperation;
    bool MemtoReg;
    bool MemRead;
    bool Branch;
    bool Jump;
    bool RegDst;
    int op_code[6];
    bool stall=false;
};

class RF{ //this class simulates registers
    public: 
        int Reg_data;
      RF(){ 
        Registers.resize(32); //32 registers  
        Registers[0] = (0);} //set all register to 0 
        
        int readRF(int Reg_addr) {   //reading the value from register
            Reg_data = Registers[Reg_addr];
            return Reg_data;}
    
        void writeRF(int Reg_addr, int Wrt_reg_data){//writting the value to register
            Registers[Reg_addr] = Wrt_reg_data;}
     
    void outputRF(int n){//saving the state of registers to text file
      ofstream rfout;
      rfout.open("RFresult.txt",std::ios_base::app);
      if (rfout.is_open()){
        rfout<<"State of Registers at cycle " << n << ":\t" <<endl;
        for (int j = 0; j<32; j++)       
          rfout << Registers[j]<<endl;}
      else cout<<"Unable to open file";
      rfout.close();} 
    private:
        vector<int>Registers;
};

class DM{    //this class simulates data memory
    public: 
        int data_MEM;
      DM(){ //500 memory addresses
        Registers.resize(500);  
        Registers[0] = (0);} //set all to 0 
        
        int readDM(int addr){//reading a value from memory
            data_MEM = Registers[addr];
            return data_MEM;}
    
        void writeDM(int addr, int Wrt_data){//writing a value to memory
            Registers[addr] = Wrt_data;}
     
    void outputDM(int n){//saving the state of the memory to text file
      ofstream rfout;
      rfout.open("DMresult.txt",std::ios_base::app);
      if (rfout.is_open()){
        rfout<<"State of Data Memory at cycle " << n << ":\t"<<endl;
        for (int j = 0; j<201; j++)        
          rfout << Registers[j]<<endl;}
      else cout<<"Unable to open file";
      rfout.close();} 
    private:
        vector<int>Registers; 
};

int ALU(int a, int b, bool& zero, int op_code);
void checkALU(int a, int b, bool& zero, int op_code);
void ReadIns(int instruction[], char InstrMem[], int PC);
int BinToDec(int sum[], int size, bool u2);
int ALUcontrol(int function, int op_code_ID);
void control(Control& ctr_ID, int op_code_ID);
void forwardingUnit(int register_addr_MEM, int register_addr_WB, int EX_rt, int EX_rs, int& forwardA, int& forwardB, Control ctr_WB, Control ctr_MEM);
void HazardDetectionUnit(int opcode, bool& stall_command);

void HazardDetectionUnit(int opcode, bool& stall_command){ //detects need to stall
    if(opcode==4 || opcode==5)
        stall_command=true;
    else
        stall_command=false;
}

void forwardingUnit(int register_addr_MEM, int register_addr_WB, int EX_rt, int EX_rs, int& forwardA, int& forwardB, Control ctr_WB, Control ctr_MEM){ //detects need of forwarding
    if(ctr_WB.RegWrite && (register_addr_WB!=0) && !(ctr_MEM.RegWrite && (register_addr_MEM!=0) && (register_addr_MEM==EX_rs)) && (register_addr_WB==EX_rs)){     
        forwardA=1;   //MEM hazard at first argument to the ALU
        cout << "Forward A1" << endl;
    }
    else if(ctr_WB.RegWrite && (register_addr_WB!=0) && !(ctr_MEM.RegWrite && (register_addr_MEM!=0) && (register_addr_MEM==EX_rt)) && (register_addr_WB==EX_rt)){        
        forwardB=1;    //MEM hazard at second argument to the ALU
        cout << "Forward B1" << endl;
    }
    else if(ctr_MEM.RegWrite && (register_addr_MEM!=0) && !(ctr_WB.RegWrite && (register_addr_WB!=0) && (register_addr_WB==EX_rs)) && (register_addr_MEM==EX_rs)){        
        forwardA=2; //EX hazard at first argument to the ALU
        cout << "Forward A2" << endl;
    }
    else if(ctr_MEM.RegWrite && (register_addr_MEM!=0) && !(ctr_WB.RegWrite && (register_addr_WB!=0) && (register_addr_WB==EX_rt)) && (register_addr_MEM==EX_rt)){       
        forwardB=2;     //EX hazard at second argument to the ALU
        cout << "Forward B2" << endl;
    }
    else{       //no forwarding
        forwardA=0;
        forwardB=0;
    }
}

void control(Control& ctr_ID, int op_code_ID){  //function seting the control signals
    ctr_ID.RegWrite=false;
    ctr_ID.ALUSrc=false;
    ctr_ID.MemWrite=false;
    ctr_ID.MemtoReg=false;
    ctr_ID.MemRead=false;
    ctr_ID.Branch=false;
    ctr_ID.Jump=false;
    ctr_ID.RegDst=false;    
    switch(op_code_ID){
        case 0:                 //R-type operations
            ctr_ID.ALUSrc=false;
            ctr_ID.RegDst=true;
            ctr_ID.MemtoReg=true;
            ctr_ID.RegWrite=true;
            break;
        case 4:                     //branch if equal
            ctr_ID.Branch=true;
            break;
        case 5:                     //branch if not equal
            ctr_ID.Branch=true;
            break;
        case 8:                     //immediate addition (with overflow)
            ctr_ID.ALUSrc=true;
            ctr_ID.RegDst=false;
            ctr_ID.MemtoReg=true;
            ctr_ID.RegWrite=true;
            break;
        case 9:                     //immediate addition (without overflow))
            ctr_ID.ALUSrc=true;
            ctr_ID.RegDst=false;
            ctr_ID.MemtoReg=true;
            ctr_ID.RegWrite=true;
            break;
        case 12:                    //immediate AND 
            ctr_ID.ALUSrc=true;
            ctr_ID.RegDst=false;
            ctr_ID.MemtoReg=true;
            ctr_ID.RegWrite=true;
            break;
        case 13:                     //immediate OR
            ctr_ID.ALUSrc=true;
            ctr_ID.RegDst=false;
            ctr_ID.MemtoReg=true;
            ctr_ID.RegWrite=true;
            break;
        case 14:                     //immediate XOR
            ctr_ID.ALUSrc=true;
            ctr_ID.RegDst=false;
            ctr_ID.MemtoReg=true;
            ctr_ID.RegWrite=true;
            break;
        case 35:                    //load word
            ctr_ID.MemRead=true;
            ctr_ID.ALUSrc=true;
            ctr_ID.RegDst=false;
            ctr_ID.MemtoReg=false;
            ctr_ID.RegWrite=true;
            break;
        case 43:                    //store word
            ctr_ID.MemWrite=true;
            ctr_ID.ALUSrc=true;
            break;
        default:                    //default all false
            ctr_ID.RegWrite=false;
            ctr_ID.ALUSrc=false;
            ctr_ID.MemWrite=false;
            ctr_ID.MemtoReg=false;
            ctr_ID.MemRead=false;
            ctr_ID.Branch=false;
            ctr_ID.Jump=false;
            ctr_ID.RegDst=false;
            break;
    }
}

int main() 
{ 
    FILE *fptr; 
    char filename[100], c, InstrMem[5000];
    int i=0;
    printf("Enter the filename to open \n"); 
    scanf("%s", filename);                     //Program ask for name of file
  
    fptr = fopen(filename, "r");                //and then open it
    if (fptr == NULL)                       //if there is no such file in that directory
    {                                       //program print out "Cannot open the file"
        printf("Cannot open file \n"); 
        exit(0); 
    } 
    c = fgetc(fptr);                //otherwise, data form file is saved in InstrMem until the EOF (end of file)
    while (c != EOF) 
    {  
        InstrMem[i]=c;
        i++;
        c = fgetc(fptr);  
    } 
    fclose(fptr);

    bool zero=false, zero_MEM=false;    //instantaite all variables needed for infinite loop
    RF myRF;
    DM myDM;
    int a_ID,b_ID,a_EX,b_EX;
    InstMemDec IM,EX;
    int op_code_ID,op_code_EX,op_code_MEM,op_code_ALU,addr1,addr2;
    int instruction[32], instruction_to_decode[32], function_EX[5];
    int PC=0, PC_ID, PC_EX;
    int ALU_result, MEM_ALU_result;
    Control ctr_ID;
    Control ctr_EX;
    Control ctr_MEM;
    Control ctr_WB;
    int MEM_addr, MEM_data_in, MEM_data_out, WB_data_out;
    bool branch=false;
    int branch_addr_MEM, branch_addr_EX;
    int register_addr_EX, register_addr_MEM, register_addr_WB;
    bool stall=false, stall_command=false;
    int forwardA=0, forwardB=0;
    int WB_out;
    int EX_stall=-1, MEM_stall=-1, WB_stall=-1, ID_stall=-1;
    bool jump=false, stall_info_ID=false, stall_info_EX=false, stall_info_MEM=false, stall_info_WB=false, correct=false;
    bool wrong_decision_stall=false, WB_stalling=false;
    int n=0;
    
    for(;;){    //endless loop
        ++n;    //increamenting cycle count
        cout << "------------CYCLE " << n << "------------" << endl;

        cout << "**FETCH**" << endl;
        //cout<<"jump "<<jump<<endl;
        //cout<<"branch "<<branch<<endl;
        if(jump){   //if the prediction about branching was incorrect
            PC=BinToDec(IM.immediate,16,true)+PC_EX;
            stall=false;    //set PC to correct value and stop stalling
            //cout << "PREDICTION"<<endl;
        }
        else if(branch){    //if the prediction is to branch
            PC=branch_addr_MEM;
            stall=false;
            //cout<<"BRANCH"<<endl;
        }
        else if(stall==false)   //normal execution - when there is no stall
            PC++;
        cout << "PC is " << PC << endl;
        ReadIns(instruction,InstrMem,PC);   //reading current instruction - according to PC value
        cout << "Instruction: ";    //display current instruction
        for(int i=0;i<32;i++)
                cout << instruction[i];
        cout << endl;
        if(stall && !jump)      //inform about stall
             cout << "/////STALL/////" << endl;

        if(n!=1){
            cout << "**DECODE**" << endl;
            IM.decode(instruction_to_decode);   //decode the instruction
            op_code_ID = BinToDec(IM.op_code,6,false);  //convert the opcode to integer
            if(stall_info_ID)       //check for stalls
                cout << "/////STALL/////" << endl;
            else if(wrong_decision_stall){
                cout << "/////STALL/////" << endl;
                EX_stall=2;
                wrong_decision_stall=false;
            }
            HazardDetectionUnit(op_code_ID, stall_command); //set the hazard control signals
            addr1 = BinToDec(IM.readRegister1addr,5,false); //convert register address value to integer
            addr2 = BinToDec(IM.readRegister2addr,5,false);
            cout << "Opcode: " << op_code_ID << "\nRegister_1 address: " << addr1 << "\nRegister_2 address: " << addr2 << endl;
            a_ID=myRF.readRF(addr1);    //read the data saved in register
            b_ID=myRF.readRF(addr2);
            cout << "Read_data_1 " << a_ID << "\nRead_data_2 " << b_ID << endl;
            control(ctr_ID, op_code_ID);    //set all control signals

            if(n>2){
                cout << "**EXECUTE**" << endl;  //set the forwarding signals
                forwardingUnit(register_addr_MEM, register_addr_WB, BinToDec(EX.readRegister2addr,5,false), BinToDec(EX.readRegister1addr,5,false), forwardA, forwardB, ctr_WB, ctr_MEM);
                op_code_ALU=ALUcontrol(BinToDec(EX.function,6,false),op_code_EX);    //set the ALU execution type signal
                //cout << "ALU opcode " << op_code_ALU << endl;
                if(stall_info_EX)   //check for stalls
                    cout << "/////STALL/////" << endl;
                else if(EX_stall>0){
                    cout << "/////STALL/////" << endl;
                    EX_stall--;
                    MEM_stall=2;
                }
                else{   //if there is no stalls
                    if(ctr_WB.RegWrite){    //set the correct values for ALU arguments
                        if(ctr_WB.MemtoReg)
                            WB_out=MEM_ALU_result;
                        else
                            WB_out=WB_data_out;}
                    if(ctr_EX.ALUSrc){  //immediate operations
                        if(forwardA==0){
                            ALU_result=ALU(a_EX,BinToDec(EX.immediate,16,false),zero,op_code_ALU);
                            cout << "Immediate operation no forwarding" << endl;
                            cout<<a_EX<<" "<<BinToDec(EX.immediate,16,false)<<endl;
                        }
                        else if(forwardA==1){
                            ALU_result=ALU(WB_out,BinToDec(EX.immediate,16,false),zero,op_code_ALU);
                            cout << "Immediate operation forwardA=1" << endl;
                        }
                        else if(forwardA==2){
                            ALU_result=ALU(MEM_addr,BinToDec(EX.immediate,16,false),zero,op_code_ALU);
                            cout << "Immediate operation forwardA=2" << endl;
                        }
                    }
                    else if(ctr_EX.ALUSrc==0){ //all operations excluding immediate operation
                        if(forwardA==0 && forwardB==0){ //no forwarding
                            ALU_result=ALU(a_EX,b_EX,zero,op_code_ALU);
                            cout << "No forwarding" << endl;
                            cout<<a_EX<<" "<<b_EX<<endl;
                        }
                        else if(forwardA==1 && forwardB==0){ //MEM hazard in first argument
                            ALU_result=ALU(WB_out,b_EX,zero,op_code_ALU);
                            cout << "forwardA=1" << endl;
                            }
                        else if(forwardA==0 && forwardB==1){ //MEM hazard in second argument
                            ALU_result=ALU(a_EX,WB_out,zero,op_code_ALU);
                            cout << "forwardB=1" << endl;
                        }
                        else if(forwardA==2 && forwardB==0){ //EX hazard in first argument
                            ALU_result=ALU(MEM_addr,b_EX,zero,op_code_ALU);
                            cout << "forwardA=2" << endl;
                        }
                        else if(forwardA==0 && forwardB==2){ //EX hazard in second argument
                            ALU_result=ALU(a_EX,MEM_data_in,zero,op_code_ALU);
                            cout << "forwardB=2" << endl;
                        }
                    }
                    
                    cout << "ALU_result " << ALU_result << "\nZero " << zero << endl;
                }
                branch_addr_MEM=branch_addr_EX; //simulating register for branch_addr signal
                branch_addr_EX=BinToDec(EX.immediate,16,true)+PC_EX;    //updating branch_addr signal for jump
                if(ctr_EX.RegDst)   //deciding what should be address for saving to the registers
                    register_addr_EX=BinToDec(EX.rt,5,false);
                else
                    register_addr_EX=BinToDec(EX.readRegister2addr,5,false);

                if(n>3){
                    cout << "**MEMORY**" << endl;
                    //cout << "op_code_MEM "<<op_code_MEM<<endl;
                    //cout<<"op_code_EX "<<op_code_EX<<endl;
                    //cout<<"op_code_ID "<<op_code_ID<<endl;
                    //cout<<"stall_info_MEM "<<stall_info_MEM<<endl;
                    //cout<<"MEM_stall "<<MEM_stall<<endl;
                    //cout<<"wrong_decision_stall "<<wrong_decision_stall<<endl;

                    if(op_code_MEM==4)
                        branch=ctr_MEM.Branch && zero_MEM;
                    else if(op_code_MEM==5)
                        branch=ctr_MEM.Branch && !zero_MEM; //branch signal decides about branching
                    else
                        branch=false;
                    if(stall_info_MEM){                  //it should be high when control for branching is on and there is correct result in ALU
                        cout << "/////STALL/////" << endl;  //check for stalls
                        ctr_MEM.RegWrite=false;}
                    else if(MEM_stall>0 && !wrong_decision_stall){
                        cout << "/////STALL/////" << endl;
                        MEM_stall--;
                        WB_stall=2;
                        ctr_MEM.RegWrite=false;
                        branch=false;
                    }
                    else{
                        if(correct && branch){   //if the prediction about jump was correct
                            branch=false;   //then stop the processor from branching again
                            cout<<"Prediction about jump was correct"<<endl;}
                        else if(correct && !branch){ //there should have been no jump back, but there was
                            branch_addr_MEM=PC_EX;  //set correct value that will be loaded to PC
                            branch=true;               //control signal for branching high
                            wrong_decision_stall=true;  //stall due to wrong prediction
                            cout<<"There should have been no jump, but there was"<<endl;
                            }
                        else if(!correct && branch){ //if there should have been jump, but there was not
                            wrong_decision_stall=true;  //stall few blocks. Don't need to worry about jump, because branch is already high
                            cout<<"There should have been jump, but there was not"<<endl;
                                }
                        if(zero_MEM==0 && ctr_MEM.Branch==1)    //stop stalling when the result of ALU doesn't allow for jump 
                            stall=false;
            
                       //if there is no stalls
                        if(ctr_MEM.MemWrite){   //and save signal is high
                            myDM.writeDM(MEM_addr,MEM_data_in); //save the ALU result in memory
                            cout << "Saving " << MEM_data_in << " in address " << MEM_addr << endl;
                        }
                        if(ctr_MEM.MemRead){    //if read addres is high
                            MEM_data_out=myDM.readDM(MEM_addr);//read data from memory with address=ALU result
                            cout << "Reading " << MEM_data_out << " from address " << MEM_addr << endl;
                        }
                    }
                    

                    if(n>4){
                    cout << "**WRITE BACK**" << endl;
                    if(stall_info_WB)   //check for stalls
                        cout << "/////STALL/////" << endl;
                    else if(WB_stall>0 && !wrong_decision_stall){
                        cout << "/////STALL/////" << endl;
                        WB_stall--;
                        if(WB_stall==0)
                            WB_stalling=false;
                    }   //if there is no stalls
                    else if(ctr_WB.RegWrite){   //and write control signal is high
                        if(ctr_WB.MemtoReg) //choose whether to save ALU result
                            WB_out=MEM_ALU_result;
                        else        //or data read from the data memory
                            WB_out=WB_data_out; 
                        myRF.writeRF(register_addr_WB, WB_out); //saving the chosen data to register
                        cout << "Saving " << WB_out << " in register address " << register_addr_WB << endl;
                    }
                }
                }
                }



            }
        WB_data_out=MEM_data_out;       //simulation of registers between stages
        register_addr_WB=register_addr_MEM; //it is done by forwarding the signal
        register_addr_MEM=register_addr_EX;//from one stage to another
        for(int i=0;i<32;i++)               //after every clock cycle 
                instruction_to_decode[i]=instruction[i];
        MEM_data_in=b_EX;
        a_EX=a_ID;
        b_EX=b_ID;
        EX=IM;
        ctr_WB=ctr_MEM;
        ctr_MEM=ctr_EX;
        stall_info_WB=stall_info_MEM;
        stall_info_MEM=stall_info_EX;
        stall_info_EX=stall_info_ID;
        correct=jump;

        if(stall_command){  //setting all control signals of ID/EX stage to 0
                ctr_EX=ctr_ID;  //when stall occurs
                op_code_MEM=op_code_EX;
                op_code_EX=op_code_ID;  
                ctr_ID.Jump=false;
                ctr_ID.Branch=false;
                ctr_ID.MemtoReg=false;
                ctr_ID.MemRead=false;
                ctr_ID.MemWrite=false;
                ctr_ID.ALUSrc=false;
                ctr_ID.RegWrite=false;
                ctr_ID.RegDst=false;
                ctr_ID.MemtoReg=false;
                ctr_ID.stall=true;
                stall=true;
                //op_code_ID=6;
            }
        else{   //if there is no stall
            ctr_EX=ctr_ID;  //normal simulation of registeres
            op_code_MEM=op_code_EX;
            op_code_EX=op_code_ID;
            ctr_ID.stall=false;
        }
        if(stall_command){  //predicting the jump
            if((BinToDec(IM.immediate,16,true))>0)
                stall=false;    //if forward jump, predict no jump
            else if((BinToDec(IM.immediate,16,true))<0){
                jump=true;      //if backward jump, predict jump
                stall_info_ID=true;
            }
        }    
        else{   //if the stall_command is low, there is no need
            stall_info_ID=false;    //to predict the jump
            jump=false;
        }
        MEM_ALU_result=MEM_addr;    //register simulation continued
        MEM_addr=ALU_result;
        zero_MEM=zero;
        PC_EX=PC_ID;
        PC_ID=PC;

        //if(n>82019){ //start observing from cycle greater than
        string input;   //stoping program execution after each clock cycle
        getline(cin, input);
            //myRF.outputRF(n); //print the state of registers or
            //myDM.outputDM(n); //data memory to text file
        //}
    }

    return 0;
}
int BinToDec(int sum[], int size, bool u2){   //function changing binary result to decimal
    int dec=0;
    int j=size-1;
    for (int i = 0; i < (size); i++) //repeat for all bits
    {
        if(i==0 && u2)             
            dec=-(sum[i]*pow(2,j)); //MSB is negative only for u2 code
        else
            dec+=(sum[i]*pow(2,j)); //add to dec bit(0 or 1) times 2 to power of
        --j;                        //the place of this bit (LSB [i=7] power 0, LSB+1 power 1 and so on)
    }
    return dec;
}

void checkALU(int a, int b, bool& zero, int op_code)
{
    cout << "Result: " << ALU(a,b,zero,op_code) << ". Is the result zero: ";
    if(zero)
        cout<<"Yes"<<endl;
    else
        cout<<"No"<<endl;
}

int ALU(int a, int b, bool& zero, int op_code)
{
     switch(op_code){
        case 0:                 //add
            if((a+b)==0) 
                zero=true;
            else
                zero=false;
            return a+b;
        case 1:                 //subtract a-b
            if((a-b)==0)
                zero=true;
            else 
                zero=false;

            return a-b;
        case 2:                 //subtract b-a
            if((b-a)==0)
                zero=true;
            else 
                zero=false;
            return b-a;
        case 3:                 //OR
            if((b|a)==0)
                zero=true;
            else 
                zero=false;
            return b|a;
        case 4:                 //AND
            if((b&a)==0)
                zero=true;
            else 
                zero=false;
            return b&a;
        case 5:                 //XOR
            if((b^a)==0)
                zero=true;
            else 
                zero=false;
            return a^b;
        case 6:
            cout << "\nALU operation NOP\n";
            zero=false;
            return 0;
     }
    
}

void ReadIns(int instruction[], char InstrMem[], int PC)    //reading instruction form instruction memory
{
    int addr=PC*33; //PC determines the addres of instruction
    for(int i=0;i<32;i++)   //exery instruction is 32 bits long
    {
        instruction[i]=((int)InstrMem[(addr+i)]-48);
    }
}

int ALUcontrol(int function, int op_code_ID){
    if(op_code_ID==0){          //R-type instructions
        switch(function){
            /*case 0:     //Shift left logical
            case 1:
            case 2:     //Shift right logical
            case 3:     //Shift right arithmetic
            case 4:     //Shift left logical variable
            case 5:
            case 6:     //Shift right logical variable
            case 7:     //Shift right arithmetic variable
            case 8:     //Jump register
            case 9:     //Jump and link register
            case 10:    //Move conditional zero
            case 11:    //Move conditional not zero
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:    //Move from hi
            case 17:    //Move to hi
            case 18:    //Move from lo
            case 19:    //Move to lo
            case 20:  
            case 21:        
            case 22:
            case 23:
            case 24:        //multiply
            case 25:        //unsigned multiply
            case 26:        //divide with overflow
            case 27:        //divide without overflow
            case 28:
            case 29:
            case 30:
            case 31:*/
            case 32:        //addition with overflow
                return 0; 
            case 33:        //addition without overflow
                return 0;
            case 34:        //Subtract (with overflow)
                return 1;
            case 35:        //Subtract (without overflow)
                return 1;
            case 36:
                return 4;   //AND
            case 37:        //OR    //should go to 37 if I repear the 6 at the bottom where the "IM.function" is copied
                return 3;
            case 38:  
                return 5;      //Exclusive OR
            /*case 39:        //NOR
            case 42:        //Set less than
            case 43:        //Set less than unsigned
            case 48:        //Trap if greater equal
            case 49:        //Unsigned trap if greater equal
            case 50:        //Trap if less than
            case 51:        //Unsigned trap if less than
            case 52:        //Trap if equal
            case 54:        //Trap if not equal*/
    }

}
    //if(op_code_ID==0)   //no operation
        //return 6;
    if(op_code_ID==8 || op_code_ID==9)  //immediate addition
        return 0;
    if(op_code_ID==12)  //immediate AND
        return 4;
    if(op_code_ID==13)  //immediate OR
        return 3;
    if(op_code_ID==14)  //immediate XOR
        return 5;
    if(op_code_ID==43)  //store
        return 0;
    if(op_code_ID==35)  //load
        return 0;
    if(op_code_ID==4)  //branch if equal
        return 1;
    if(op_code_ID==5)  //branch if not equal
        return 1;


}

