# Using the Disassembler

## Running the Disassembler 

Moira includes a built-in disassembler that converts instructions, the program counter, and the status register into human-readable text. The following API functions generate textual representations:

- `int disassemble(char *str, u32 addr) const`

   Disassembles the instruction at the specified address. The caller must ensure that the target string is initialized and large enough to hold the output. The function returns how many bytes the disassembled instruction occupies in memory, making it easy to know where the next instruction starts.

- `void disassembleSR(char *str, const StatusRegister &sr) const;`

    Creates a textual representation for the referenced status register state. If the second parameter is omitted, the current state of the status register is disassembled.

- `void dump8(char *str, u8 value) const`
- `void dump16(char *str, u16 value) const`
- `void dump24(char *str, u32 value) const`
- `void dump32(char *str, u32 value) const`
    
   These functions generate a textual representation of the provided integer value.

    
## Customizing Output

The disassembler output can be customized in various ways by calling one of the following API functions:

- `void setDasmSyntax(Syntax value)` 

    Moira supports the following syntax styles: 
    - `MOIRA` (default)
        Standard Motorola syntax.
    - `MOIRA_MIT` 
        A Moira-specific variation using MIT syntax.
    - `GNU`
        Mimics the output of the m68k disassembler in `binutils`.
    - `GNU_MIT`
        Mimics the output of `binutils` when run in MIT mode. 
    - `MUSASHI`
        Reproduces the exact output of the Musashi disassembler.

- `void setDasmNumberFormat(DasmNumberFormat value)`

    Controls the formatting of numbers in disassembly output. The structure below defines the configurable parameters:

    ````c++
    struct DasmNumberFormat
    {
        const char *prefix;         // Prefix for hexidecimal numbers
        u8 radix;                   // 10 (decimal) or 16 (hexadecimal)
        bool upperCase;             // Lettercase for hexadecimal digits A...F
        bool plainZero;             // Determines whether 0 is printed with a prefix
    };
    ````

- `void setDasmLetterCase(LetterCase value)`

    The letter case determines the appearance of mnemonics and register names. The following options are availabe: 
    - `MIXED_CASE`
        Uses default case based on the selected syntax style.
    - `LOWER_CASE`
        Outputs mnemonics and register names in lowercase.
    - `UPPER_CASE`
        Outputs mnemonics and register names in uppercase.
    
- `void setDasmIndentation(int value)`

    Adjusts the spacing between the mnemonic and the rest of the instruction for better readability.

## Examples

- Running Moira in vAmiga with syntax style `MOIRA`
    ![Screenshot](../images/dasm_mot.png)

- Running Moira in vAmiga with syntax style `MOIRA_MIT`
    ![Screenshot](../images/dasm_mit.png)

