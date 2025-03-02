# Instruction Handlers

This article explains the inner workings of a typical instruction handler in Moira. All instruction handlers are stored in a large lookup table. As explained in the previous article, Moira retrieves the appropriate handler from this table and executes it via a function pointer.
To understand how a typical instruction handler is structured, let's examine the implementation of the ABCD instruction:

![ABCD instruction details](../images/abcd.png)

Below is the handler implementing the register addressing variant of the ABCD instruction:

```c++
template <Core C, Instr I, Mode M, Size S> void
Moira::execAbcdRg(u16 opcode)
{
    AVAILABILITY(Core::C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = bcd<C, I, Byte>(readD<Byte>(src), readD<Byte>(dst));

    prefetch<C, POLL>();
    SYNC(2);
    writeD<Byte>(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 6,  6,  4,        0,  0,  0,        0,  0,  0)

    FINALIZE
}
```

## Template Parameters 

All instruction handlers are declared with four template arguments:
- `Core C`: Specifies the emulated CPU core
- `Instr I`: Specifies the emulated instruction
- `Mode M`: Specifies the addressing mode
- `Size S`: Specifies the instruction's data size

For the `ABCD Dy,Dx` instruction, instruction, the template arguments resolve as follows:
- `C` equals `C68000`, `C68010` or `C68020`, depending on the emulated CPU model
- `I` equals `ABCD`
- `M` equals `DN`
- `S` equals `Byte`

## Execution Flow

**1. Availability Check**

   All instruction handlers begin with the `AVAILABILITY` macro:
   ```c++
   AVAILABILITY(C68000)
   ```
   This macro performs a series of assertion checks to validate the C template parameter. Additionally, it calls the `willExecute` function if the instruction is among those being observed, as specified in `MoiraConfig.h`.

**2. Extracting Operands**

The next lines extract the source and destination register indices from the opcode:
```c++
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
```

**3. Performing the Operation**

The bcd function computes the result of the ABCD operation using the values from the source and destination registers:

```c++
    u32 result = bcd<C, I, Byte>(readD<Byte>(src), readD<Byte>(dst));
```

**4. Prefetching the Next Instruction**

Moira fills the prefetch queue:

```c++
    prefetch<C, POLL>();
    SYNC(2);
```

**5. Writing Back the Result**

```c++
    writeD<Byte>(dst, result);
```

**6. Cycle Timing Adjustments**

    The following section is only relevant in simple cycle mode:
```c++
    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 6,  6,  4,        0,  0,  0,        0,  0,  0)
```
In simple cycle mode `CYCLES_DN` expands to a suitable call of `sync`. In our example, it expands to `sync(6)` when emulating a M68000 or M68010, and to `sync(4)` for all other models. In precision timing mode, the macro expands into an empty statement as the `sync` calls have already been issued in the correct places in the instruction handler code.

**7. Finalization**

The handler ends with the FINALIZE macro:

```
    FINALIZE
```
This macro ensures that the `didExecute` function is called if Moira has been configured to do so in `MoiraConfig.h`.
 
