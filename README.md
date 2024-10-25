# 5-Stage Pipelined RISC-V Processor Simulator

This project is a C++ implementation of a simplified 5-stage pipelined RISC-V processor simulator. It models the basic functionalities of a processor pipeline, including instruction fetching, decoding, execution, memory access, and write-back stages. The simulator handles basic arithmetic and memory operations, along with hazard detection and simple forwarding mechanisms.

## Features

- **Pipeline Stages**: Implements the IF, ID, EX, MEM, and WB stages of a processor pipeline.
- **Instruction Support**: Supports a subset of RISC-V instructions, including:
  - R-type: `ADD`, `SUB`, `AND`, `OR`, `XOR`
  - I-type: `ADDI`, `LW`, `XORI`, `ORI`, `ANDI`
  - S-type: `SW`
  - B-type: `BEQ`, `BNE`
  - J-type: `JAL`
- **Hazard Detection**: Basic hazard detection and stalling for load-use hazards.
- **Forwarding**: Implements simple forwarding to handle data hazards.
- **Memory and Register Files**: Simulates instruction memory, data memory, and register file operations.
- **Output Logs**: Generates detailed logs of the register file and pipeline states after each cycle.

## Getting Started

### Prerequisites

- C++ compiler supporting C++11 standard or later (e.g., GCC, Clang).

### Compilation

Compile the simulator using a C++ compiler. For example:

```bash
g++ -std=c++11 -o simulator main.cpp
