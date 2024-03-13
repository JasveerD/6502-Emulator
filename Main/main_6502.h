#include <iostream>
#include <cstdlib>

// http://www.6502.org/users/obelisk/

using Byte = unsigned char;     // 1 Byte = 8 bits
using Word = unsigned short;    // 1 word = 16 bits
using u32 = unsigned int;       // 32 bit unsigned int
using s32 = signed int;         // 32-bit signed int

struct Mem{
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialise(){
        for(u32 i = 0; i < MAX_MEM; i++){
            Data[i] = 0;
        }
    }

    Byte operator[](u32 Address) const{      // read 1 byte
        // assert here, Address < MAX_MEM
        return Data[Address];
    }

    Byte &operator[](u32 Address){      // write 1 byte
        // assert here, Address < MAX_MEM
        return Data[Address];
    }

    void WriteWord(Word Value, u32 Address, s32 &Cycles){    // writes a 16 bit data
        Data[Address] = Value & 0xFF ;
        Data[Address+1] = (Value >> 8);
        Cycles -= 2;
    }
};

struct CPU{

    Word PC;    // PC -> program counter
    Word SP;    // SP -> stack pointer

    Byte A, X, Y;   // registers

    // status flags
    Byte C : 1;     // carry flag
    Byte Z : 1;     // zero flag
    Byte I : 1;     // interrupt disable
    Byte D : 1;     // decimal mode
    Byte B : 1;     // break command
    Byte V : 1;     // overflow flag
    Byte N : 1;     // negative flag

    void Reset(Mem &memory){
        PC = 0xFFFC;
        SP = 0x0100;
        C = Z = I = D = B = V = N = 0;
        A = X = Y = 0;

        memory.Initialise();
    }

    Byte FetchByte(s32 &Cycles, Mem &memory){   // fetches 8 bits of data
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Word FetchWord(s32 &Cycles, Mem &memory){   // fetches 16 bits of data
        // 6502 is little endian
        // keeps the least significant address at the smallest memory location.
        Word Data = memory[PC];
        PC++;
        Data |= (memory[PC] << 8);
        PC++;
        Cycles-=2;
        return Data;
    }

    Byte ReadByte(s32 &Cycles, Word Address, Mem &memory){  // reads 8 bits of data
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    }

    Word ReadWord(s32 &Cycles, Word Address, Mem &memory){  // reads 8 bits of data
        Byte LoByte = ReadByte(Cycles, Address, memory);
        Byte HiByte = ReadByte(Cycles, Address+1, memory);
        return LoByte | (HiByte << 8);
    }

    // opcodes
    static constexpr Byte
        // LDA - Load Accumulator
        INS_LDA_IM = 0xA9,  // load into A, immediate mode
        INS_LDA_ZP = 0xA5,  // load into A, zero page
        INS_LDA_ZPX = 0xB5,
        INS_LDA_ABS = 0xAD,
        INS_LDA_ABSX = 0xBD,
        INS_LDA_ABSY = 0xB9,
        INS_LDA_INDX = 0xA1,
        INS_LDA_INDY = 0xB1,


        // JSR - Jump to Subroutine
        INS_JSR = 0x20;     // jump to subroutine


    void LDASetStatus(){
        Z = (A==0);     // setting zero flag if A==0
        N = (A & 0b10000000) > 0;   // setting negative flag if bit 7 of A = 0
    }


    s32 Execute(s32 Cycles, Mem &memory){
        const s32 CyclesRequested = Cycles;
        while(0<Cycles){
            Byte Ins = FetchByte(Cycles, memory);   // Fetching Instruction
            switch (Ins){

                case INS_LDA_IM:{
                    Byte Value = FetchByte(Cycles, memory);
                    A = Value;
                    LDASetStatus();
                }break;

                case INS_LDA_ZP:{
                    Byte ZeroPageAddress = FetchByte(Cycles, memory);
                    A = ReadByte(Cycles, ZeroPageAddress, memory);
                    LDASetStatus();
                }break;

                case INS_LDA_ZPX:{
                    Byte ZeroPageAddress = FetchByte(Cycles, memory);
                    ZeroPageAddress += X;
                    Cycles--;
                    A = ReadByte(Cycles, ZeroPageAddress, memory);
                    LDASetStatus();
                }break;

                case INS_LDA_ABS:{
                    Word AbsAddress = FetchWord(Cycles, memory);
                    A = ReadByte(Cycles, AbsAddress, memory);
                    LDASetStatus();
                }break;

                case INS_LDA_ABSX:{
                    Word AbsAddress = FetchWord(Cycles, memory);
                    Word AbsAddressX = AbsAddress += X;
                    A = ReadByte(Cycles, AbsAddressX, memory);
                    if(AbsAddressX - AbsAddress >= 0xFF){
                        Cycles--;
                    }
                    LDASetStatus();
                }break;

                case INS_LDA_ABSY:{
                    Word AbsAddress = FetchWord(Cycles, memory);
                    Word AbsAddressY = AbsAddress += Y;
                    A = ReadByte(Cycles, AbsAddressY, memory);
                    if(AbsAddressY - AbsAddress >= 0xFF){
                        Cycles--;
                    }
                    LDASetStatus();
                }break;

                case INS_LDA_INDX:{
                    Byte ZPAddress = FetchByte(Cycles, memory);
                    ZPAddress += X;
                    Cycles--;
                    Word EffectiveAddress = ReadWord(Cycles, ZPAddress, memory);
                    A = ReadByte(Cycles, EffectiveAddress, memory);
                    LDASetStatus();
                }break;

                case INS_LDA_INDY:{
                    Byte ZPAddress = FetchByte(Cycles, memory);
                    Word EffectiveAddress = ReadWord(Cycles, ZPAddress, memory);
                    Word EffectiveAddrY = EffectiveAddress + Y;
                    A = ReadByte(Cycles, EffectiveAddrY, memory);
                    if(EffectiveAddrY - EffectiveAddress >= 0xFF){
                        Cycles--;
                    }
                    LDASetStatus();
                }break;

                case INS_JSR:{
                    Word SubAddr = FetchWord(Cycles, memory);   //SubAddr -> subroutine addr
                    memory.WriteWord(PC-1, SP, Cycles);
                    SP += 2;
                    PC = SubAddr;
                    Cycles--;
                }break;

                default:{
                    printf("Instruction not handled %d\n", Ins);
                    throw -1;
                }
            }
        }
        const s32 NumCyclesUsed = CyclesRequested - Cycles;
        return NumCyclesUsed;
    }
};

