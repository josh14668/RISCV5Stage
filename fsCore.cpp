#include<iostream>
#include<string>
#include<vector>
#include<bitset>
#include<fstream>
#include<algorithm>

using namespace std;

#define MemSize 1000

template<size_t N, size_t F>
std::bitset<F> extendBitset(const std::bitset<N>& original) {
    static_assert(F > N, "Target bit size must be greater than original bit size.");
    
    std::bitset<F> extended;
    bool signBit = original[N - 1]; // Get the sign bit (the most significant bit of the original bitset)
    
    // Copy the original bits to the extended bitset
    for (size_t i = 0; i < N; ++i) {
        extended[i] = original[i];
    }
    
    // Perform sign extension if necessary
    if (signBit) {
        for (size_t i = N; i < F; ++i) {
            extended.set(i);
        }
    }
    
    return extended;
}

template<std::size_t B>
long bitset_to_long(const std::bitset<B>& b) {
    struct {long x:B;} s;
    return s.x = b.to_ulong();
}

struct IFStruct {
    bitset<32>  PC;
    bool        nop = false;
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop = true;
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<32>  Imm;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bitset<3>   funct3;
    bitset<7>   funct7;
    bool        is_I_type = false;
    bool        rd_mem = false;
    bool        wrt_mem = false;
    bool        alu_op;     //1 for add, 0 for sub
    bool        wrt_enable = false;
    bool        nop = true;
    bool        is_branch = false;
    bool        is_jal = false;
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem = false;
    bool        wrt_mem = false;
    bool        wrt_enable = false;
    bool        nop = true;
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable = false;
    bool        nop = true;
};

struct stateStruct {
    IFStruct    IF;
    IDStruct    ID;
    EXStruct    EX;
    MEMStruct   MEM;
    WBStruct    WB;
};

class InsMem
{
    public:
        string id, ioDir;
        InsMem(string name, string ioDir) {
            id = name;
            IMem.resize(MemSize);
            ifstream imem;
            string line;
            int i=0;
            imem.open(ioDir+"/imem.txt");
            if (imem.is_open())
            {
                while (getline(imem,line))
                {   
                    IMem[i] = bitset<8>(line.substr(0, 8));
                    i++;
                }
            }
            else cout<<"Unable to open IMEM input file.";
            imem.close();
        }

        bitset<32> readInstr(bitset<32> ReadAddress) {
            bitset<32> instruction;
            size_t startIndex = static_cast<size_t>((ReadAddress.to_ulong()) % MemSize);
            for (int i = 0; i < 4; ++i) 
            {
                instruction = (instruction << 8) | IMem[startIndex + i].to_ulong();
            }
            return instruction;
        }

    private:
        vector<bitset<8> > IMem;
};

class DataMem
{
    public:
        string id, opFilePath, ioDir;
        DataMem(string name, string ioDir) : id(name), ioDir(ioDir) {
            DMem.resize(MemSize);
            opFilePath = ioDir + "/" + name + "_DMEMResult.txt";
            ifstream dmem;
            string line;
            int i=0;
            dmem.open(ioDir + "/dmem.txt");
            if (dmem.is_open())
            {
                while (getline(dmem,line))
                {
                    DMem[i] = bitset<8>(line.substr(0,8));
                    i++;
                }
            }
            else cout<<"Unable to open DMEM input file.";
            dmem.close();
        }

        bitset<32> readDataMem(bitset<32> Address) {
            bitset<32> data;
            size_t startIndex = static_cast<size_t>((Address.to_ulong()) % MemSize);
            for (int i = 0; i < 4; ++i) 
            {
                data = (data << 8) | DMem[startIndex + i].to_ulong();
            }
            return data;
        }

        void writeDataMem(bitset<32> Address, bitset<32> WriteData) 
        {
            size_t startIndex = static_cast<size_t>((Address.to_ulong()) % MemSize);
            for (int i = 0; i < 4; ++i)
            {
                DMem[startIndex + i] = bitset<8>((WriteData.to_ulong() >> (8 * (3 - i))) & 0xFF);
            }
        }

        void outputDataMem() 
        {
            ofstream dmemout;
            dmemout.open(opFilePath, std::ios_base::trunc);
            if (dmemout.is_open()) {
                for (int j = 0; j< MemSize; j++)
                {
                    dmemout << DMem[j]<<endl;
                }

            }
            else cout<<"Unable to open "<<id<<" DMEM result file." << endl;
            dmemout.close();
        }

    private:
        vector<bitset<8> > DMem;
};

class RegisterFile
{
    public:
        string outputFile;
        RegisterFile(string ioDir): outputFile (ioDir + "/RFResult.txt") {
            Registers.resize(32);
            Registers[0] = bitset<32> (0);
        }

        bitset<32> readRF(bitset<5> Reg_addr) {
            return Registers[Reg_addr.to_ulong()];
        }

        void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
            if (Reg_addr.to_ulong() != 0) // x0 is always zero
                Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
        }

        void outputRF(int cycle) {
            ofstream rfout;
            if (cycle == 0)
                rfout.open(outputFile, std::ios_base::trunc);
            else
                rfout.open(outputFile, std::ios_base::app);
            if (rfout.is_open())
            {
                rfout<<"State of RF after executing cycle:\t"<<cycle<<endl;
                for (int j = 0; j<32; j++)
                {
                    rfout << Registers[j]<<endl;
                }
            }
            else cout<<"Unable to open RF output file."<<endl;
            rfout.close();
        }

    private:
        vector<bitset<32> >Registers;
};

class Core {

    public:
        RegisterFile myRF;
        uint32_t cycle = 0;
        bool halted = false;
        string ioDir;
        struct stateStruct state, nextState;
        InsMem ext_imem;
        DataMem* ext_dmem;

        Core(string ioDir, InsMem &imem, DataMem* dmem): myRF(ioDir), ioDir(ioDir), ext_imem (imem), ext_dmem (dmem) {
            state.IF.PC = 0;
        }

        virtual void step() {}

        virtual void printState() {}

        virtual void PerformanceMetrics() {}

};

class FiveStageCore : public Core{
    public:
        FiveStageCore(string ioDir, InsMem &imem, DataMem *dmem): Core(ioDir + "/FS_", imem, dmem), opFilePath(ioDir + "/StateResult_FS.txt")
        {
            state.IF.nop = false;
            state.ID.nop = true;
            state.EX.nop = true;
            state.MEM.nop = true;
            state.WB.nop = true;
        }

        void step() {

            /* --------------------- WB stage --------------------- */
            if(!state.WB.nop){
                WB_STAGE();
            }

            /* --------------------- MEM stage -------------------- */
            if(!state.MEM.nop){
                MEM_STAGE();
            }
            nextState.WB = state.WB;
            nextState.WB.nop = state.MEM.nop;

            /* --------------------- EX stage --------------------- */
            if(!state.EX.nop){
                EX_STAGE();
            }
            nextState.MEM = state.MEM;
            nextState.MEM.nop = state.EX.nop;

            /* --------------------- ID stage --------------------- */
            if(!state.ID.nop){
                ID_STAGE();
            }
            nextState.EX = state.EX;
            nextState.EX.nop = state.ID.nop;

            /* --------------------- IF stage --------------------- */
            if(!state.IF.nop){
                IF_STAGE();
            }
            nextState.ID = state.ID;
            nextState.ID.nop = state.IF.nop;

            if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop)
            {
                halted = true;
            }

            myRF.outputRF(cycle); // dump RF
            printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 

            state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
            cycle++;
        }

        void IF_STAGE()
        {
            if (!state.IF.nop) {
                nextState.IF.PC = state.IF.PC.to_ulong() + 4;
                nextState.ID.Instr = ext_imem.readInstr(state.IF.PC);
                nextState.ID.nop = false;

                // Check for stall
                if (EX_hazard_detected) {
                    nextState.ID = state.ID; // Stall
                    nextState.IF = state.IF; // Stall PC
                }
            }
        }

        void ID_STAGE()
        {
            bitset<7> opcode = bitset<7>((state.ID.Instr.to_ulong()) & 0x7F);

            // Initialize control signals
            nextState.EX.is_I_type = false;
            nextState.EX.rd_mem = false;
            nextState.EX.wrt_mem = false;
            nextState.EX.alu_op = true; // Default to ADD
            nextState.EX.wrt_enable = false;
            nextState.EX.is_branch = false;
            nextState.EX.is_jal = false;

            switch(opcode.to_ulong())
            {
                case 0x33: // R-Type
                    decodeRType(state.ID.Instr);
                    break;
                case 0x03: // I-Type Load
                case 0x13: // I-Type ALU
                    decodeIType(state.ID.Instr, opcode);
                    break;
                case 0x63: // B-Type Branch
                    decodeBType(state.ID.Instr);
                    break;
                case 0x23: // S-Type Store
                    decodeSType(state.ID.Instr);
                    break;
                case 0x6F: // J-Type JAL
                    decodeJType(state.ID.Instr);
                    break;
                case 0x7F: // HALT (Assuming 0x7F as HALT)
                    halted = true;
                    nextState.IF.nop = true;
                    nextState.ID.nop = true;
                    nextState.EX.nop = true;
                    nextState.MEM.nop = true;
                    nextState.WB.nop = true;
                    break;
                default:
                    // Handle unknown opcode
                    nextState.EX.nop = true;
                    break;
            }

            // Hazard detection
            detectHazards();
        }

        void EX_STAGE()
        {
            if (state.EX.is_branch) {
                // Branch execution
                bool branch_taken = false;
                if (state.EX.funct3 == 0x0) { // BEQ
                    if (state.EX.Read_data1 == state.EX.Read_data2)
                        branch_taken = true;
                } else if (state.EX.funct3 == 0x1) { // BNE
                    if (state.EX.Read_data1 != state.EX.Read_data2)
                        branch_taken = true;
                }
                if (branch_taken) {
                    nextState.IF.PC = state.EX.Imm.to_ulong() + state.EX.Read_data1.to_ulong();
                    // Flush the pipeline
                    nextState.ID.nop = true;
                    nextState.EX.nop = true;
                }
            } else if (state.EX.is_jal) {
                // JAL execution
                nextState.EX.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Imm.to_ulong();
                nextState.IF.PC = nextState.EX.ALUresult.to_ulong();
                nextState.EX.Wrt_reg_addr = state.EX.Wrt_reg_addr;
                nextState.EX.wrt_enable = true;
                nextState.EX.Read_data1 = bitset<32>(state.IF.PC.to_ulong() + 4); // Return address
                // Flush the pipeline
                nextState.ID.nop = true;
            } else {
                // ALU operations
                if (state.EX.is_I_type) {
                    nextState.MEM.ALUresult = bitset<32>(state.EX.Read_data2.to_ulong() + state.EX.Imm.to_ulong());
                } else {
                    if (state.EX.alu_op) // ADD
                        nextState.MEM.ALUresult = bitset<32>(state.EX.Read_data1.to_ulong() + state.EX.Read_data2.to_ulong());
                    else // SUB
                        nextState.MEM.ALUresult = bitset<32>(state.EX.Read_data1.to_ulong() - state.EX.Read_data2.to_ulong());
                }
            }

            nextState.MEM.Store_data = state.EX.Read_data2;
            nextState.MEM.Rs = state.EX.Rs;
            nextState.MEM.Rt = state.EX.Rt;
            nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
            nextState.MEM.rd_mem = state.EX.rd_mem;
            nextState.MEM.wrt_mem = state.EX.wrt_mem;
            nextState.MEM.wrt_enable = state.EX.wrt_enable;
            nextState.MEM.nop = state.EX.nop;
        }

        void MEM_STAGE()
        {
            if (state.MEM.rd_mem) {
                nextState.WB.Wrt_data = ext_dmem->readDataMem(state.MEM.ALUresult);
            } else if (state.MEM.wrt_mem) {
                ext_dmem->writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
            } else {
                nextState.WB.Wrt_data = state.MEM.ALUresult;
            }

            nextState.WB.Rs = state.MEM.Rs;
            nextState.WB.Rt = state.MEM.Rt;
            nextState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
            nextState.WB.wrt_enable = state.MEM.wrt_enable;
            nextState.WB.nop = state.MEM.nop;
        }

        void WB_STAGE()
        {
            if (state.WB.wrt_enable) {
                myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
            }
        }

        void detectHazards()
        {
            EX_hazard_detected = false;

            // Load-use hazard detection
            if (state.EX.rd_mem && !state.EX.nop) {
                bitset<5> ex_rd = state.EX.Wrt_reg_addr;
                bitset<5> id_rs1 = (state.ID.Instr.to_ulong() >> 15) & 0x1F;
                bitset<5> id_rs2 = (state.ID.Instr.to_ulong() >> 20) & 0x1F;

                if (ex_rd != 0 && (ex_rd == id_rs1 || ex_rd == id_rs2)) {
                    // Stall the pipeline
                    nextState.ID = state.ID;
                    nextState.IF = state.IF;
                    nextState.EX.nop = true;
                    EX_hazard_detected = true;
                }
            }
        }

        void decodeRType(const std::bitset<32>& instruction)
        {
            // Extracting fields from the instruction
            std::bitset<7> funct7((instruction.to_ulong() >> 25) & 0x7F);
            std::bitset<5> rs2((instruction.to_ulong() >> 20) & 0x1F);
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);
            std::bitset<5> rd((instruction.to_ulong() >> 7) & 0x1F);

            nextState.EX.Rs = rs1;
            nextState.EX.Rt = rs2;
            nextState.EX.Wrt_reg_addr = rd;
            nextState.EX.funct3 = funct3;
            nextState.EX.funct7 = funct7;
            nextState.EX.is_I_type = false;
            nextState.EX.rd_mem = false;
            nextState.EX.wrt_mem = false;
            nextState.EX.wrt_enable = true;
            nextState.EX.nop = false;

            // Read data from register file
            nextState.EX.Read_data1 = myRF.readRF(rs1);
            nextState.EX.Read_data2 = myRF.readRF(rs2);

            // Determine ALU operation
            if (funct3 == 0x0 && funct7 == 0x00) {
                nextState.EX.alu_op = true; // ADD
            } else if (funct3 == 0x0 && funct7 == 0x20) {
                nextState.EX.alu_op = false; // SUB
            } else {
                // Handle other R-type instructions
            }
        }

        void decodeIType(const std::bitset<32>& instruction, bitset<7> opcode) 
        {
            std::bitset<12> imm(instruction.to_ulong() >> 20);
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);
            std::bitset<5> rd((instruction.to_ulong() >> 7) & 0x1F);

            nextState.EX.Rs = rs1;
            nextState.EX.Rt = 0;
            nextState.EX.Wrt_reg_addr = rd;
            nextState.EX.funct3 = funct3;
            nextState.EX.is_I_type = true;
            nextState.EX.wrt_enable = true;
            nextState.EX.nop = false;

            nextState.EX.Imm = extendBitset<12,32>(imm);
            nextState.EX.Read_data2 = myRF.readRF(rs1);

            if (opcode == 0x03) { // Load instruction
                nextState.EX.rd_mem = true;
                nextState.EX.wrt_mem = false;
            } else { // I-type ALU instruction
                nextState.EX.rd_mem = false;
                nextState.EX.wrt_mem = false;
            }
        }

        void decodeBType(const std::bitset<32>& instruction) 
        {
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<5> rs2((instruction.to_ulong() >> 20) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);

            // Immediate
            std::bitset<1> imm11 = (instruction >> 7)[0];
            std::bitset<4> imm4_1 = (instruction.to_ulong() >> 8) & 0xF;
            std::bitset<6> imm10_5 = (instruction.to_ulong() >> 25) & 0x3F;
            std::bitset<1> imm12 = (instruction >> 31)[0];

            int32_t imm = (imm12.to_ulong() << 12) | (imm11.to_ulong() << 11) | (imm10_5.to_ulong() << 5) | (imm4_1.to_ulong() << 1);
            if (imm12.to_ulong()) {
                imm |= 0xFFFFE000; // Sign-extend negative immediate
            }

            nextState.EX.Rs = rs1;
            nextState.EX.Rt = rs2;
            nextState.EX.funct3 = funct3;
            nextState.EX.is_branch = true;
            nextState.EX.nop = false;

            nextState.EX.Imm = bitset<32>(imm);

            nextState.EX.Read_data1 = myRF.readRF(rs1);
            nextState.EX.Read_data2 = myRF.readRF(rs2);
        }

        void decodeSType(const std::bitset<32>& instruction) 
        {
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<5> rs2((instruction.to_ulong() >> 20) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);

            // Immediate
            std::bitset<5> imm4_0 = (instruction.to_ulong() >> 7) & 0x1F;
            std::bitset<7> imm11_5 = (instruction.to_ulong() >> 25) & 0x7F;

            int32_t imm = (imm11_5.to_ulong() << 5) | imm4_0.to_ulong();
            if (imm11_5[6]) {
                imm |= 0xFFFFF000; // Sign-extend negative immediate
            }

            nextState.EX.Rs = rs1;
            nextState.EX.Rt = rs2;
            nextState.EX.funct3 = funct3;
            nextState.EX.is_I_type = true;
            nextState.EX.wrt_mem = true;
            nextState.EX.rd_mem = false;
            nextState.EX.wrt_enable = false;
            nextState.EX.nop = false;

            nextState.EX.Imm = bitset<32>(imm);

            nextState.EX.Read_data1 = myRF.readRF(rs1);
            nextState.EX.Read_data2 = myRF.readRF(rs2);
        }

        void decodeJType(const std::bitset<32>& instruction) 
        {
            std::bitset<5> rd((instruction.to_ulong() >> 7) & 0x1F);

            // Immediate
            std::bitset<1> imm20 = (instruction >> 31)[0];
            std::bitset<8> imm19_12 = (instruction.to_ulong() >> 12) & 0xFF;
            std::bitset<1> imm11 = (instruction >> 20)[0];
            std::bitset<10> imm10_1 = (instruction.to_ulong() >> 21) & 0x3FF;

            int32_t imm = (imm20.to_ulong() << 20) | (imm19_12.to_ulong() << 12) | (imm11.to_ulong() << 11) | (imm10_1.to_ulong() << 1);
            if (imm20.to_ulong()) {
                imm |= 0xFFF00000; // Sign-extend negative immediate
            }

            nextState.EX.Rs = 0;
            nextState.EX.Rt = 0;
            nextState.EX.Wrt_reg_addr = rd;
            nextState.EX.is_jal = true;
            nextState.EX.wrt_enable = true;
            nextState.EX.nop = false;

            nextState.EX.Imm = bitset<32>(imm);
            nextState.EX.Read_data1 = bitset<32>(state.IF.PC.to_ulong());
        }

        void printState(stateStruct state, int cycle) {
            ofstream printstate;
            if (cycle == 0)
                printstate.open(opFilePath, std::ios_base::trunc);
            else 
                printstate.open(opFilePath, std::ios_base::app);
            if (printstate.is_open()) {

                printstate<<"=========================================================================\t"<<endl;

                printstate<<"State after executing cycle:\t"<<cycle<<endl; 

                printstate<<"----------------------  IF  ---------------------\t"<<endl;

                printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;        
                printstate<<"IF.nop:\t"<<state.IF.nop<<endl; 

                printstate<<"----------------------  ID  ---------------------\t"<<endl;

                printstate<<"ID.Instr:\t"<<state.ID.Instr<<endl; 
                printstate<<"ID.nop:\t"<<state.ID.nop<<endl;

                printstate<<"----------------------  EX  ---------------------\t"<<endl;

                printstate<<"EX.Read_data1:\t"<<state.EX.Read_data1<<endl;
                printstate<<"EX.Read_data2:\t"<<state.EX.Read_data2<<endl;
                printstate<<"EX.Imm:\t"<<state.EX.Imm<<endl; 
                printstate<<"EX.Rs:\t"<<state.EX.Rs<<endl;
                printstate<<"EX.Rt:\t"<<state.EX.Rt<<endl;
                printstate<<"EX.Wrt_reg_addr:\t"<<state.EX.Wrt_reg_addr<<endl;
                printstate<<"EX.is_I_type:\t"<<state.EX.is_I_type<<endl; 
                printstate<<"EX.rd_mem:\t"<<state.EX.rd_mem<<endl;
                printstate<<"EX.wrt_mem:\t"<<state.EX.wrt_mem<<endl;        
                printstate<<"EX.alu_op:\t"<<state.EX.alu_op<<endl;
                printstate<<"EX.wrt_enable:\t"<<state.EX.wrt_enable<<endl;
                printstate<<"EX.nop:\t"<<state.EX.nop<<endl;        

                printstate<<"----------------------  MEM  ---------------------\t"<<endl;

                printstate<<"MEM.ALUresult:\t"<<state.MEM.ALUresult<<endl;
                printstate<<"MEM.Store_data:\t"<<state.MEM.Store_data<<endl; 

                printstate<<"MEM.Rs:\t"<<state.MEM.Rs<<endl;
                printstate<<"MEM.Rt:\t"<<state.MEM.Rt<<endl;   
                printstate<<"MEM.Wrt_reg_addr:\t"<<state.MEM.Wrt_reg_addr<<endl;              
                printstate<<"MEM.rd_mem:\t"<<state.MEM.rd_mem<<endl;
                printstate<<"MEM.wrt_mem:\t"<<state.MEM.wrt_mem<<endl; 
                printstate<<"MEM.wrt_enable:\t"<<state.MEM.wrt_enable<<endl;         
                printstate<<"MEM.nop:\t"<<state.MEM.nop<<endl;        

                printstate<<"----------------------  WB  ---------------------\t"<<endl;

                printstate<<"WB.Wrt_data:\t"<<state.WB.Wrt_data<<endl;
                printstate<<"WB.Rs:\t"<<state.WB.Rs<<endl;
                printstate<<"WB.Rt:\t"<<state.WB.Rt<<endl;
                printstate<<"WB.Wrt_reg_addr:\t"<<state.WB.Wrt_reg_addr<<endl;
                printstate<<"WB.wrt_enable:\t"<<state.WB.wrt_enable<<endl;
                printstate<<"WB.nop:\t"<<state.WB.nop<<endl; 
            }
            else cout<<"Unable to open FS StateResult output file." << endl;
            printstate.close();
        }
    private:
        string opFilePath;
        bool EX_hazard_detected = false;
};

int main(int argc, char* argv[]) {

    string ioDir = "";
    if (argc == 1) {
        cout << "Enter path containing the memory files: ";
        cin >> ioDir;
    }
    else if (argc > 2) {
        cout << "Invalid number of arguments. Machine stopped." << endl;
        return -1;
    }
    else {
        ioDir = argv[1];
        cout << "IO Directory: " << ioDir << endl;
    }

    InsMem imem = InsMem("Imem", ioDir);
    DataMem dmem_fs = DataMem("FS", ioDir);
    FiveStageCore FSCore(ioDir, imem, &dmem_fs);

    while (!FSCore.halted) {
        FSCore.step();
    }

    FSCore.ext_dmem->outputDataMem();

    return 0;
}
