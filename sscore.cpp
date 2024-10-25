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
    bool        nop;
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop;
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<32>  Imm;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bitset<3>   funct3;
    bool        is_I_type;
    bool        rd_mem;
    bool        wrt_mem;
    bool        alu_op;     //1 for addu, lw, sw, 0 for subu
    bool        wrt_enable;
    bool        nop;
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem;
    bool        wrt_mem;
    bool        wrt_enable;
    bool        nop;
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable;
    bool        nop;
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

			bitset<32> singleInt;
			size_t startIndex = static_cast<size_t>(ReadAddress.to_ulong());
			for (size_t i = 0; i < 4; ++i) 
			{
				singleInt <<= 8;
				singleInt |= IMem[startIndex+i].to_ulong();
        	
    		}
			
			return singleInt;
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
            bitset<32> singleData;
			size_t startIndex = static_cast<size_t>(Address.to_ulong());
			for (size_t i = 0; i < 4; ++i) 
			{
				singleData <<= 8;
              
				singleData |= DMem[startIndex+i].to_ulong();
         

    		}
			
			return singleData;
			
		}

        void writeDataMem(bitset<32> Address, bitset<32> WriteData) 
        {
            size_t index = static_cast<size_t>(Address.to_ulong());

           
            
            for(size_t i = 0; i<4;i++)
            {   
            
                DMem[index+i].reset();
                DMem[index+i] |= bitset<8>(WriteData.to_string().substr(0,8));
                WriteData <<= 8;
                
            }
        }

        void outputDataMem() 
        {
            ofstream dmemout;
            dmemout.open(opFilePath, std::ios_base::trunc);
            if (dmemout.is_open()) {
                for (int j = 0; j< 1000; j++)
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
     	RegisterFile(string ioDir): outputFile (ioDir + "RFResult.txt") {
			Registers.resize(32);
			Registers[0] = bitset<32> (0);
        }

        bitset<32> readRF(bitset<5> Reg_addr) {
            return Registers[Reg_addr.to_ulong()];
        }

        void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
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
        enum SpecificInstruction {
            ///// R-types /////
            ADD,  
            SUB,  
            XOR,
            OR,
            AND,

            /// I-Types //////
            ADDI, // I-type
            XORI,
            ORI,
            ANDI,
            LW,   // I-type

            SW,   // S-type

            //B-Type///
            BEQ,  
            BNE,

            JAL,  // J-type
            HALT,
            NOP,  // Not an operation
        };
        
		RegisterFile myRF;
		uint32_t cycle = 0;
		bool halted = false;
		string ioDir;
		struct stateStruct state, nextState;
		InsMem ext_imem;
		DataMem* ext_dmem;

		Core(string ioDir, InsMem &imem, DataMem* dmem): myRF(ioDir), ioDir(ioDir), ext_imem (imem), ext_dmem (dmem) {}

		virtual void step() {}

		virtual void printState() {}

        virtual void PerformanceMetrics() {}

        SpecificInstruction decodeRType(const std::bitset<32> instruction)
        {

            // Extracting fields from the instruction
            std::bitset<7> funct7(instruction.to_ulong() >> 25);
            std::bitset<5> rs2((instruction.to_ulong() >> 20) & 0x1F);
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);
            std::bitset<5> rd((instruction.to_ulong() >> 7) & 0x1F);

            nextState.EX.Imm = 0; // R-type instructions don't use an immediate
            nextState.EX.Rs = rs1.to_ulong();
            nextState.EX.Rt = rs2.to_ulong();
         
            //Update Read date with contents of register file.
            nextState.EX.Read_data1 = myRF.readRF(state.EX.Rs);
            nextState.EX.Read_data2 = myRF.readRF(state.EX.Rt);

            nextState.EX.Wrt_reg_addr = rd.to_ulong();

            nextState.EX.is_I_type = false; // R-type, not I-type
            nextState.EX.rd_mem = false; // R-type instructions don't read from memory
            nextState.EX.wrt_mem = false; // R-type instructions don't write to memory
            // Assuming alu_op represents whether the operation is an addition or subtraction
            // This simplification might not accurately reflect all R-type operations. Adjust as needed.

            nextState.EX.alu_op = (funct7.to_ulong() == 0x20); // False for ADD etc, True for SUB 

            nextState.EX.wrt_enable = true; // R-type instructions typically write to a register
            nextState.EX.nop = state.ID.nop; // Not a NOP if we're decoding an actual instruction


           // TODO: Set HALT flags 

            nextState.EX.funct3 = funct3;

            if(state.EX.alu_op == true)
            {
                return SpecificInstruction::SUB;
            }
            else
            {
                switch (funct3.to_ulong())
                {
                case 0b00: return SpecificInstruction::ADD;
        
                case 0b100: return SpecificInstruction::XOR;
                   
                case 0b110: return SpecificInstruction::OR;
                    
                case 0b111: return SpecificInstruction::AND;
            
                }

            }
            return NOP;
            
        }

        SpecificInstruction decodeIType(const std::bitset<32> instruction, bitset<7> opcode) 
        {
            std::bitset<12> imm(instruction.to_ulong() >> 20);
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);
            std::bitset<5> rd((instruction.to_ulong() >> 7) & 0x1F);

            nextState.EX.Imm = extendBitset<12,32>(imm);

            nextState.EX.Rs = rs1.to_ulong();

            //state.EX.Read_data1 = signExtendedImm;
            nextState.EX.Read_data2 = myRF.readRF(state.EX.Rs);
            
            nextState.EX.Rt = 0; // Not used in I-type
            nextState.EX.Wrt_reg_addr = rd.to_ulong(); //Destination register

            nextState.EX.is_I_type = true;
            nextState.EX.rd_mem = (funct3.to_ulong() == 0x0 && opcode.to_ulong() == 0x03); // For LW instruction funct3 = 0x00, opcode = 0x03, since there is a read from memory 

            nextState.EX.wrt_mem = false; //No write to memory in I-type 

            // Assuming alu_op represents add for immediate addition and load
            nextState.EX.alu_op = true; // All requires alu op

            nextState.EX.wrt_enable = true; //All I type writes to reg file
            nextState.EX.nop = false;

            nextState.EX.funct3 = funct3;

            if(nextState.EX.rd_mem == true)
            {
                nextState.EX.wrt_enable = false;
                return SpecificInstruction::LW;
            }
            else
            {
                switch (funct3.to_ulong())
                {
                case 0b00: return SpecificInstruction::ADDI;
                    
                case 0b100: return SpecificInstruction::XORI;
                  
                case 0b110: return SpecificInstruction::ORI;
                   
                case 0b111: return SpecificInstruction::ANDI;
                
                default: return SpecificInstruction::NOP;
                   
                }
            }
            return NOP;
        }

        SpecificInstruction decodeJType(const std::bitset<32> instruction) 
        {
            // Extracting parts of the immediate spread across the instruction
            int32_t imm20 = (instruction[31] ? 1 : 0);
            int32_t imm10_1 = (instruction.to_ulong() >> 21) & 0x3FF;
            int32_t imm11 = (instruction[20] ? 1 : 0);
            int32_t imm19_12 = (instruction.to_ulong() >> 12) & 0xFF;
            
            // Assembling the immediate and sign-extending it
            int32_t imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
            // Sign-extension
            if (imm20) imm |= 0xFFF00000; // If imm[20] is set, extend the sign
            
            std::bitset<5> rd((instruction.to_ulong() >> 7) & 0x1F); // Destination register

            // Update the EXStruct
            nextState.EX.Imm = std::bitset<32>(imm & 0xFFFF); // Assuming Imm can hold the lower 16 bits, adjust as needed
            nextState.EX.Wrt_reg_addr = rd;
            nextState.EX.is_I_type = false; // Not an I-type
            nextState.EX.rd_mem = false; // Doesn't read from memory
            nextState.EX.wrt_mem = false; // Doesn't write to memory
            nextState.EX.alu_op = false; // Not an ALU operation
            nextState.EX.wrt_enable = true; // Writes to register, stores PC+4 in rd
            nextState.EX.nop = state.ID.nop; // Not a NOP

            myRF.writeRF(state.EX.Wrt_reg_addr.to_ulong(), state.IF.PC.to_ulong()+4);
            state.IF.PC = state.IF.PC.to_ulong() + state.EX.Imm.to_ulong();    

            
            nextState.ID.nop = true;
            nextState.ID.Instr = bitset<32>(0);

            return SpecificInstruction::JAL; // JAL is the specific J-type instruction being decoded
        }

        SpecificInstruction decodeBType(const std::bitset<32> instruction) 
        {
            // Extracting parts of the immediate and reassembling it
            int32_t imm12 = (instruction[31] ? 1 : 0);
            int32_t imm10_5 = (instruction.to_ulong() >> 25) & 0x3F;
            int32_t imm4_1 = (instruction.to_ulong() >> 8) & 0xF;
            int32_t imm11 = (instruction[7] ? 1 : 0);
            
            // Assembling the immediate and sign-extending it
            int32_t imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);

          
            // Sign-extension
            if (imm12) imm |= 0xFFFFE000; // If imm[12] is set, extend the sign
            
            
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<5> rs2((instruction.to_ulong() >> 20) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);

            // Update the EXStruct
            nextState.EX.Imm = std::bitset<32>(imm & 0xFFFF); // Assuming Imm can hold the lower 16 bits
           

            nextState.EX.Rs = rs1;
            nextState.EX.Rt = rs2;
            nextState.EX.is_I_type = false;
            nextState.EX.rd_mem = false; // B-type doesn't read from memory
            nextState.EX.wrt_mem = false; // B-type doesn't write to memory
            nextState.EX.alu_op = false; // Not directly an ALU operation, but involves comparison
            nextState.EX.wrt_enable = false; // B-type doesn't write to register
            nextState.EX.nop = state.ID.nop;


            // Determine the specific B-type instruction

            if(nextState.EX.Rs == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data1 = nextState.WB.Wrt_data;
            }
            if(nextState.EX.Rt == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data2 = nextState.WB.Wrt_data;
            }

            if(nextState.EX.Rs == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data1 = nextState.MEM.ALUresult;
            }
            if(nextState.EX.Rt == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data2 = nextState.MEM.ALUresult;
            }

             //Load use hazard

            if(nextState.MEM.Wrt_reg_addr == nextState.EX.Rs || nextState.MEM.Wrt_reg_addr == nextState.EX.Rt)
            {
                nextState.EX.nop = true;
                nextState.ID = state.ID;
                nextState.IF.PC = state.IF.PC.to_ulong()-4;
            }else
            {

            switch (funct3.to_ulong()) {
                case 0x0:

                if(myRF.readRF(nextState.EX.Rs) == myRF.readRF(nextState.EX.Rt))
                {
                    nextState.IF.PC = state.IF.PC.to_ulong() - 4;
                    nextState.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm);

                    nextState.ID.nop = true;
                    nextState.ID.Instr = bitset<32>(0);

                }
                break;

                case 0x1:

                if(myRF.readRF(nextState.EX.Rs) != myRF.readRF(nextState.EX.Rt))
                {
                    nextState.IF.PC = state.IF.PC.to_ulong() - 4;
                    nextState.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm);

                    nextState.ID.nop = true;
                    nextState.ID.Instr = bitset<32>(0);
                }            
                break;
            }


            }


        }
        
        SpecificInstruction decodeSType(const std::bitset<32> instruction) 
        {
            // Extracting parts of the immediate and reassembling it
            int32_t imm11_5 = (instruction.to_ulong() >> 25) & 0x7F;
            int32_t imm4_0 = (instruction.to_ulong() >> 7) & 0x1F;
        

            // Assembling the immediate and sign-extending it
            int32_t imm = (imm11_5 << 5) | imm4_0;

            
            // Sign-extension
            if (imm & 0x800) imm |= 0xFFFFF000; // If the sign bit (bit 11) is set, extend the sign
            

            
            std::bitset<5> rs1((instruction.to_ulong() >> 15) & 0x1F);
            std::bitset<5> rs2((instruction.to_ulong() >> 20) & 0x1F);
            std::bitset<3> funct3((instruction.to_ulong() >> 12) & 0x7);


            // Update the EXStruct
            nextState.EX.Imm = std::bitset<32>(imm & 0xFFFF); // Assuming Imm can hold the lower 16 bits



            nextState.EX.Rs = rs1;
            nextState.EX.Rt = rs2;

      
            nextState.EX.funct3 = funct3;
            nextState.EX.is_I_type = false;
            nextState.EX.rd_mem = false; // S-type doesn't read from memory
            nextState.EX.wrt_mem = true; // S-type writes to memory
            nextState.EX.alu_op = false; // Not an ALU operation, but involves memory address calculation
            nextState.EX.wrt_enable = false; // S-type doesn't write to register file
            nextState.EX.nop = state.ID.nop;

            // Determine the specific S-type instruction
            return SpecificInstruction::SW;
        }

        SpecificInstruction ID_Stage(bitset<32> instruction)
        {
            SpecificInstruction whatInst;

            bitset<32> opcodeMask = 0b1111111;

            bitset<7> opcode((instruction & opcodeMask).to_ulong()); // Pulls opcode last 7 bits

            cout<< opcode << endl;
            
            switch(opcode.to_ulong()){

                case 0x33: return decodeRType(instruction); //R-Type 

                case 0x3: return decodeIType(instruction,opcode); //I type LW

                case 0x13: return decodeIType(instruction,opcode); //I-Type

                case 0x6F: return decodeJType(instruction); //J-Type

                case 0x63: return decodeBType(instruction);//B-Type execution happens in ID stage
        
                case 0x23: return decodeSType(instruction); //S-Type

                case 0x7F: return HALT;

                default: return NOP;
               
            }
            
           
            
        } 

        void IF_Stage()
        {
            cout<<"IF_Stage"<<endl;

            if(ext_imem.readInstr(state.IF.PC) == 0x7F)
            {
                state.IF.nop = true;
                state.EX.nop = true;
                state.ID.nop = true;
                state.MEM.nop = true;
                state.WB.nop = true;
                halted = true;
            }

            state.IF.PC = state.IF.PC.to_ulong() + 4;


        }

        void EX_Stage(SpecificInstruction instruction)
        {

            bitset<32> result;
            switch (instruction) {
            case SpecificInstruction::ADD:
                state.MEM.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Read_data2.to_ulong();
                break;
            case SpecificInstruction::SUB:
                state.MEM.ALUresult = state.EX.Read_data1.to_ulong() - state.EX.Read_data2.to_ulong();
                break;
            case SpecificInstruction::XOR:
                state.MEM.ALUresult = state.EX.Read_data1 ^ state.EX.Read_data2;
                break;
            case SpecificInstruction::OR:
                state.MEM.ALUresult = state.EX.Read_data1 | state.EX.Read_data2;
                break;
            case SpecificInstruction::AND:
                state.MEM.ALUresult = state.EX.Read_data1 & state.EX.Read_data2;
                break;

            // I-type instructions
            case SpecificInstruction::ADDI:
                //state.MEM.ALUresult = bitset_to_long(state.EX.Read_data2)+bitset_to_long(state.EX.Imm);
                state.MEM.ALUresult = state.EX.Read_data2.to_ulong() + state.EX.Imm.to_ulong();
                
                break;
            case SpecificInstruction::XORI:
                state.MEM.ALUresult = state.EX.Read_data2.to_ulong() ^ state.EX.Imm.to_ulong();
                
                break;
            case SpecificInstruction::ORI:
                state.MEM.ALUresult = state.EX.Read_data2.to_ulong() | state.EX.Imm.to_ulong();
            
                break;
            case SpecificInstruction::ANDI:
                state.MEM.ALUresult = state.EX.Read_data2.to_ulong() & state.EX.Imm.to_ulong();
                
                break;
            case SpecificInstruction::LW:

                state.MEM.ALUresult = myRF.readRF(state.EX.Rs).to_ulong() + state.EX.Imm.to_ulong();
                break;
            case SpecificInstruction::SW:

                state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
                state.MEM.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Imm.to_ulong();

                break;

            // J-type instruction
            case SpecificInstruction::JAL:
                // Example for JAL - typically involves PC update and storing return address
                myRF.writeRF(state.EX.Wrt_reg_addr.to_ulong(), state.IF.PC.to_ulong()+4);
                state.IF.PC = state.IF.PC.to_ulong() + state.EX.Imm.to_ulong();    
                // Update PC logic here
                break;

            // NOP or unknown instruction
            case SpecificInstruction::NOP:
            default:
                // No operation or handle unknown instruction
                break;
        }




        }    
};


const char* instMap[]  = {
    "ADD",  // Corresponds to SpecificInstruction::ADD
    "SUB",  // Corresponds to SpecificInstruction::SUB
    "XOR",  // Corresponds to SpecificInstruction::XOR
    "OR",   // Corresponds to SpecificInstruction::OR
    "AND",  // Corresponds to SpecificInstruction::AND

    "ADDI", // Corresponds to SpecificInstruction::ADDI
    "XORI", // Corresponds to SpecificInstruction::XORI
    "ORI",  // Corresponds to SpecificInstruction::ORI
    "ANDI", // Corresponds to SpecificInstruction::ANDI
    "LW",   // Corresponds to SpecificInstruction::LW

    "SW",   // Corresponds to SpecificInstruction::SW

    "BEQ",  // Corresponds to SpecificInstruction::BEQ
    "BNE",  // Corresponds to SpecificInstruction::BNE

    "JAL",  // Corresponds to SpecificInstruction::JAL
    "HALT",
    "NOP"   // Corresponds to SpecificInstruction::NOP
    };


class SingleStageCore : public Core {
	public:
		SingleStageCore(string ioDir, InsMem &imem, DataMem* dmem): Core(ioDir + "/SS_", imem, dmem), opFilePath(ioDir + "/StateResult_SS.txt") {}


        void PerformanceMetrics(uint32_t cycle, uint32_t totalInstructions){
            ofstream printMetrics;
            string filePath = ioDir + "PerformanceMetrics.txt";
            float cpi = static_cast<float>(cycle) /static_cast<float> (totalInstructions);
            float ipc = 1/static_cast<float>(cpi);
            printMetrics.open(filePath, std::ios_base::trunc);
            if (printMetrics.is_open()){
                printMetrics << "Performance of Single Stage: " << endl;
                printMetrics << "#Cycles -> " << cycle << endl;
                printMetrics << "#Instructions -> " << totalInstructions << endl;
                printMetrics << "CPI -> " << cpi << endl;
                printMetrics << "IPC -> " << ipc << endl; 
            }
            else cout << "Unable to open a performance metrics output file." << endl;
            printMetrics.close();
        }


        void EX_STAGE(SpecificInstruction instruction)
    {
        bitset<32> result;
        switch (instruction) {
        case SpecificInstruction::ADD:
            state.MEM.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Read_data2.to_ulong();
            break;
        case SpecificInstruction::SUB:
            state.MEM.ALUresult = state.EX.Read_data1.to_ulong() - state.EX.Read_data2.to_ulong();
            break;
        case SpecificInstruction::XOR:
            state.MEM.ALUresult = state.EX.Read_data1 ^ state.EX.Read_data2;
            break;
        case SpecificInstruction::OR:
            state.MEM.ALUresult = state.EX.Read_data1 | state.EX.Read_data2;
            break;
        case SpecificInstruction::AND:
            state.MEM.ALUresult = state.EX.Read_data1 & state.EX.Read_data2;
            break;

        // I-type instructions
        case SpecificInstruction::ADDI:
            //state.MEM.ALUresult = bitset_to_long(state.EX.Read_data2)+bitset_to_long(state.EX.Imm);
            state.MEM.ALUresult = state.EX.Read_data2.to_ulong() + state.EX.Imm.to_ulong();
            
            break;
        case SpecificInstruction::XORI:
            state.MEM.ALUresult = state.EX.Read_data2.to_ulong() ^ state.EX.Imm.to_ulong();
            
            break;
        case SpecificInstruction::ORI:
            state.MEM.ALUresult = state.EX.Read_data2.to_ulong() | state.EX.Imm.to_ulong();
         
            break;
        case SpecificInstruction::ANDI:
            state.MEM.ALUresult = state.EX.Read_data2.to_ulong() & state.EX.Imm.to_ulong();
            
            break;
        case SpecificInstruction::LW:

            state.MEM.ALUresult = myRF.readRF(state.EX.Rs).to_ulong() + state.EX.Imm.to_ulong();
            break;
        case SpecificInstruction::SW:

            state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
            state.MEM.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Imm.to_ulong();

            break;

        // J-type instruction
        case SpecificInstruction::JAL:
            // Example for JAL - typically involves PC update and storing return address
            myRF.writeRF(state.EX.Wrt_reg_addr.to_ulong(), state.IF.PC.to_ulong()+4);
            state.IF.PC = state.IF.PC.to_ulong() + state.EX.Imm.to_ulong();    
            // Update PC logic here
            break;

        // NOP or unknown instruction
        case SpecificInstruction::NOP:
        default:
            // No operation or handle unknown instruction
            break;
    }
        

    }

        SpecificInstruction whatInst;

        uint32_t totalInstruction = 0;

        void step() {
            
    

            ///////////////// IF Stage ///////////////////////////////////
       
            state.ID.Instr = ext_imem.readInstr(state.IF.PC); // Passes read instruction into the ID instr

            state.IF.PC = state.IF.PC.to_ulong() + 4; // Updates PC 
            totalInstruction +=1;
            
           

            bitset<32> current_instruction = state.ID.Instr;

           

            bitset<32> opcodeMask = 0b1111111;

            bitset<7> opcode((current_instruction & opcodeMask).to_ulong()); // Pulls opcode last 7 bits
  
             
		
            if(opcode == 0b1111111)
            {
                state.IF.nop = true;
                state.IF.PC = state.IF.PC.to_ulong() - 4;
                totalInstruction +=1;

            }
            ///////////////////////////////////////////////////////////////
 
            //////////////// ID Stage ////////////////////////////////////


            switch(opcode.to_ulong()){
                // case 0x7F: //Halt function and end program
                //     state.IF.nop=true;

                //     break;
                
                case 0x33: //R-Type
               
                    whatInst = decodeRType(current_instruction);

                    break;
                case 0x3: //I type LW

                    whatInst = decodeIType(current_instruction,opcode);
                    break;
                    

                case 0x13: //I-Type
                    whatInst = decodeIType(current_instruction,opcode);
                    break; 

                case 0x6F: //J-Type
                    whatInst = decodeJType(current_instruction);
                    break;
                
                case 0x63://B-Type execution happens in ID stage
                    whatInst = decodeBType(current_instruction);
              
                    if(whatInst == SpecificInstruction::BEQ)
                    {
                        if(myRF.readRF(state.EX.Rs) == myRF.readRF(state.EX.Rt))
                        {
                            state.IF.PC = state.IF.PC.to_ulong() - 4;

                            state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm);
                        }

                    }else if (whatInst == SpecificInstruction::BNE)
                    {   
                    
                       
                        if(myRF.readRF(state.EX.Rs) != myRF.readRF(state.EX.Rt))
                        {   
                           
                            state.IF.PC = state.IF.PC.to_ulong() - 4;
                            
                            state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm);           

                        }
                    }
                    break;

                case 0x23: //S-Type
                    whatInst = decodeSType(current_instruction);
                    break;

                default:
                    whatInst = NOP;
                    break;
               
            }
            
           

       

            //////////////// EXECUTE STAGE //////////////////////////////

 
            EX_STAGE(whatInst);

          
            //////////////// Mem Stage ///////////////////////////////
         

        
            if(whatInst == SpecificInstruction::SW) // S-Type 
            {
             
                ext_dmem->writeDataMem(state.MEM.ALUresult, myRF.readRF(state.EX.Rt));
                
            }

            if(whatInst == SpecificInstruction::LW) //LW
            {           
                myRF.writeRF(state.EX.Wrt_reg_addr,ext_dmem->readDataMem(state.MEM.ALUresult));
                myRF.outputRF(cycle);
            }

            /////////////////// Write Back /////////////////////////////
    

            if(state.EX.wrt_enable)
            {   
                myRF.writeRF(state.EX.Wrt_reg_addr,state.MEM.ALUresult); 
            }
        
			if (state.IF.nop && nextState.IF.nop)
            {
				halted = true;
            }

			myRF.outputRF(cycle); // dump RF
			printState(state, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ...
            
            PerformanceMetrics(cycle,totalInstruction);

			nextState = state; // The end of the cycle and updates the current state with the values calculated in this cycle

			cycle++;
   
		}
       
		void printState(stateStruct state, int cycle) {
    		ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else
    			printstate.open(opFilePath, std::ios_base::app);
    		// if (printstate.is_open()) {
    		//     printstate<<"State after executing cycle:\t"<<cycle<<endl;

    		//     printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;
    		//     printstate<<"IF.nop:\t"<<state.IF.nop<<endl;
    		// }
    		// else cout<<"Unable to open SS StateResult output file." << endl;
    		// printstate.close();
		}
	private:
		string opFilePath;
};


class FiveStageCore : public Core{
	public:

        void IF_STAGE()
        {
            nextState.IF.PC = state.IF.PC.to_ulong() + 4;
            nextState.ID.Instr = ext_imem.readInstr(state.IF.PC);
            nextState.ID.nop = state.IF.nop;
        }

        void ID_STAGE()
        {
            bitset<32> opcodeMask = 0b1111111;

            bitset<7> opcode((state.ID.Instr & opcodeMask).to_ulong()); // Pulls opcode last 7 bits
            
            switch(opcode.to_ulong())
            {
                case 0x33: //R-Type 

                    decodeRType(state.ID.Instr);
                    break;

                case 0x3: // I-type LW

                    decodeIType(state.ID.Instr,opcode);
                    break;

                case 0x13: // I-Type 

                    decodeIType(state.ID.Instr,opcode);
                    break;

                case 0x6F: //J-type 

                    decodeJType(state.ID.Instr);
                    break;

                case 0x63://B-type

                    decodeBType(state.ID.Instr);
                    break;

                case 0x23:   
                    decodeSType(state.ID.Instr);
                    break;

            }
        }

        void EX_STAGE()
        {

        //TODO: EX Stage needs to be checked.

        if(state.EX.is_I_type)
        {
            switch(state.EX.funct3.to_ulong())
            {
                case 0b00: //ADDI
                nextState.MEM.ALUresult =  bitset_to_long(state.EX.Read_data2)+bitset_to_long(state.EX.Imm);
                break;
                
                case 0b100: //XORI

                nextState.MEM.ALUresult = state.EX.Read_data2 ^ state.EX.Imm; 
                
                break;
                  
                case 0b110: //ORI
                
                nextState.MEM.ALUresult = state.EX.Read_data2 | state.EX.Imm;

                break;
                   
                case 0b111: //ANDI
                
                nextState.MEM.ALUresult = state.EX.Read_data2 & state.EX.Imm;

                break;
            
            //TODO: LW and SW

                
            }
        }
        else
        {
        
            switch(state.EX.funct3.to_ulong())
        {
            case 0b00: //ADD
                if(state.EX.alu_op)
                {
                    nextState.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Read_data2);
                }else
                {
                    nextState.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Read_data2);
                }
            break;

            case 0b100: //XOR
                nextState.MEM.ALUresult = state.EX.Read_data1 ^ state.EX.Read_data2;
            
            break;
            
            case 0b110: //OR
                nextState.MEM.ALUresult = state.EX.Read_data1 | state.EX.Read_data2;

            break;

            case 0b111://AND
                nextState.MEM.ALUresult = state.EX.Read_data1 & state.EX.Read_data2;

            break;

            }
          

        }


        nextState.MEM.Rs = state.EX.Rs;
        nextState.MEM.Rt = state.EX.Rt;

        nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
        nextState.MEM.Store_data = state.EX.Read_data2;

        nextState.MEM.rd_mem = state.EX.rd_mem;
        nextState.MEM.wrt_mem = state.EX.wrt_mem;
        nextState.MEM.wrt_enable = state.EX.wrt_enable;

        nextState.MEM.nop = state.EX.nop;


        
        }

        void MEM_STAGE()
        {
           
            if(state.MEM.rd_mem)
            {
                nextState.WB.Wrt_data = ext_dmem->readDataMem(state.MEM.ALUresult);
            
            }
            if(state.MEM.wrt_mem)
            {
                ext_dmem->writeDataMem(state.MEM.ALUresult, myRF.readRF(state.MEM.Rt));
                
            }else
            {
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
            if(state.WB.wrt_enable)
            {
                myRF.writeRF(state.EX.Wrt_reg_addr,state.WB.Wrt_data);
            }

        }


		FiveStageCore(string ioDir, InsMem &imem, DataMem *dmem): Core(ioDir + "\\FS_", imem, dmem), opFilePath(ioDir + "\\StateResult_FS.txt")
        {
            state.IF.nop = false;
            state.ID.nop = true;
            state.EX.nop = true;
            state.MEM.nop = true;
            state.WB.nop = true;
        }


		void step() {
           
            cout<<"START 5 Stage"<<endl;
			/* Your implementation */
			/* --------------------- WB stage --------------------- */
			if(!state.WB.nop){WB_STAGE();}
			
			/* --------------------- MEM stage -------------------- */
			
			if(!state.MEM.nop){MEM_STAGE();}
			
			/* --------------------- EX stage --------------------- */
			
            if(!state.EX.nop){EX_STAGE();}
			
			/* --------------------- ID stage --------------------- */

            if(!state.ID.nop){ID_STAGE();}

            //RAW Hazards Check
          
            if(nextState.EX.Rs == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data1 = nextState.WB.Wrt_data;
            }
            if(nextState.EX.Rt == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data2 = nextState.WB.Wrt_data;
            }
            if(nextState.EX.Rs == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data1 = nextState.MEM.ALUresult;
            }
            if(nextState.EX.Rt == nextState.MEM.Wrt_reg_addr)
            {
                nextState.EX.Read_data2 = nextState.MEM.ALUresult;
            }


        //Load use hazard

            if(nextState.MEM.Wrt_reg_addr == nextState.EX.Rs || nextState.MEM.Wrt_reg_addr == nextState.EX.Rt)
            {
                nextState.EX.nop = true;
                nextState.ID = state.ID;
                nextState.IF.PC = state.IF.PC.to_ulong()-4;
            }

        
			/* --------------------- IF stage --------------------- */

            IF_STAGE();
			
			
			//halted = true;
			if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop)
				halted = true;
        
            myRF.outputRF(cycle); // dump RF
			printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
       
		     state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
			cycle++;
      
		}

		void printState(stateStruct state, int cycle) {
		    ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else 
		    	printstate.open(opFilePath, std::ios_base::app);
		    if (printstate.is_open()) {
		        printstate<<"State after executing cycle:\t"<<cycle<<endl; 

		        printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;        
		        printstate<<"IF.nop:\t"<<state.IF.nop<<endl; 

		        printstate<<"ID.Instr:\t"<<state.ID.Instr<<endl; 
		        printstate<<"ID.nop:\t"<<state.ID.nop<<endl;

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

		        printstate<<"MEM.ALUresult:\t"<<state.MEM.ALUresult<<endl;
		        printstate<<"MEM.Store_data:\t"<<state.MEM.Store_data<<endl; 
		        printstate<<"MEM.Rs:\t"<<state.MEM.Rs<<endl;
		        printstate<<"MEM.Rt:\t"<<state.MEM.Rt<<endl;   
		        printstate<<"MEM.Wrt_reg_addr:\t"<<state.MEM.Wrt_reg_addr<<endl;              
		        printstate<<"MEM.rd_mem:\t"<<state.MEM.rd_mem<<endl;
		        printstate<<"MEM.wrt_mem:\t"<<state.MEM.wrt_mem<<endl; 
		        printstate<<"MEM.wrt_enable:\t"<<state.MEM.wrt_enable<<endl;         
		        printstate<<"MEM.nop:\t"<<state.MEM.nop<<endl;        

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
  //  DataMem dmem_ss = DataMem("SS", ioDir);
	DataMem dmem_fs = DataMem("FS", ioDir);

	//SingleStageCore SSCore(ioDir, imem, &dmem_ss);
	FiveStageCore FSCore(ioDir, imem, &dmem_fs);
	

    while (1) {
		// if (!SSCore.halted)
		// 	SSCore.step();

		if (!FSCore.halted)
			FSCore.step();

		if (FSCore.halted) //&& FSCore.halted)
			break;
    }


    // dump SS and FS data mem.
    //SSCore.ext_dmem->outputDataMem();
    FSCore.ext_dmem->outputDataMem();
    

	// // dump SS and FS data mem.

	return 0;
}
