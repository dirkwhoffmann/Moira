# Configuration Options

Moira provides a couple of compile-time options. All options are defined in file `MoiraConfig.h`. 

- `MOIRA_PRECISE_TIMING`
   
   Set to true to enable precise timing mode (68000 and 68010 only).
 
   When disabled, Moira calls the 'sync' function at the end of each instruction,
   passing the number of elapsed cycles as an argument. In precise timing mode,
   'sync' is called before each memory access, allowing the client to emulate
   the surrounding hardware up to the point where the memory access occurs.
 
   Precise timing mode is available only for 68000 and 68010 emulation. For
   other supported models, this setting has no effect.
 
   Enable to improve accuracy, disable it to enhance performance.
 
- `MOIRA_VIRTUAL_API`

   Set to true to implement the CPU interface as virtual functions.
  
   To interact with the environment (e.g., for reading data from memory),
   the CPU calls an appropriate function that must be implemented by the client.
   When this option is enabled, all API functions are declared virtual, following
   the standard object-oriented programming (OOP) paradigm. However, virtual
   functions introduce a performance penalty. Setting this option to false allows
   the client API to be statically linked, improving performance.
  
   Enable to adhere to the standard OOP paradigm, disable to gain speed.
 
- `MOIRA_EMULATE_ADDRESS_ERROR` 

  Set to true to enable address error checking.
 
  The 68000 and 68010 trigger an address error violation if a word or longword
  is accessed at an odd memory address.
  
  Enable to improve accuracy, disable to gain speed.
 
- `MOIRA_EMULATE_FC`
  
  Set to true to emulate function code pins FC0 - FC2.
 
  Whenever memory is accessed, the function code pins enable external hardware
  to inspect the access type. If used, these pins are usually connected to an
  external memory management unit (MMU).
 
  Enable to improve accuracy, disable to gain speed.
 
- `MOIRA_ENABLE_DASM`

  Set to true to enable the disassembler.
 
  The disassembler requires a jump table which consumes about 1MB of memory.
 
  Disable to save space.
 
- `MOIRA_BUILD_INSTR_INFO_TABLE`

  Set to true to build the InstrInfo lookup table.
 
  The instruction info table stores information about the instruction
  (Instr I), the addressing mode (Mode M), and the size attribute (Size S) for
  all 65536 opcode words. The table is meant to provide data for, e.g.,
  external debuggers. It is not needed by Moira itself and therefore disabled
  by default.
 
  Disable to save space.
 
- `MOIRA_MIMIC_MUSASHI`

  Enables Musashi compatibility mode.
 
  When set to true, Moira runs in a special mode designed for compatibility
  with the Musashi emulator. This mode is primarily used by the test runner
  application to compare Moira's behavior against Musashi.
 
  Set to false for improved accuracy.
  
- `MOIRA_WILL_EXECUTE`
  
  This macro appears at the beginning of each instruction handler.
  Moira will call 'willExecute(...)' for all listed instructions.
 
  Example: `#define MOIRA_WILL_EXECUTE I == Instr::STOP || I == Instr::TAS || I == Instr::BKPT`

- `MOIRA_DID_EXECUTE`

  This macro appear at the end of each instruction handler.
  Moira will call 'didExecute(...)' for all listed instructions.
 
  Example: `#define MOIRA_DID_EXECUTE I == Instr::RESET`
