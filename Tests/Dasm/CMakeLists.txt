# Disassembler tests. The tools disassemble every opcode with Moira and compare the
# output with Binutils and with the disassembler of UAE (dasm) or Musashi (dasmmusashi).
# They are built from the sources of the random runner (see ../Random).
add_test_runner(dasm moiraUAE DASM)
add_test_runner(dasmmusashi moiraMusashi DASM)

add_test(NAME Dasm COMMAND dasm --seed 1)
add_test(NAME DasmMusashi COMMAND dasmmusashi --seed 1)
set_tests_properties(Dasm DasmMusashi PROPERTIES LABELS "dasm")
