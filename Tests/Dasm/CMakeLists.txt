# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

# Disassembler tests. The tools disassemble every opcode and compare the output of Moira with
# the reference disassemblers. They are built from the sources of the random runner (see ../Random).
#
#   dasmmot   Motorola syntax. Moira's GNU, MUSASHI and UAE syntaxes are compared with Binutils,
#             Musashi and UAE. Moira's own syntax is checked for plausibility only.
#   dasmmit   MIT syntax. Moira's GNU_MIT syntax is compared with Binutils (compiled without
#             MOTOROLA). Moira's own MOIRA_MIT syntax is checked for plausibility only.
add_test_runner(dasmmot moiraUAE ${CMAKE_CURRENT_SOURCE_DIR}/DasmMot.cpp binutils)
add_test_runner(dasmmit moiraUAE ${CMAKE_CURRENT_SOURCE_DIR}/DasmMit.cpp binutilsmit)

add_test(NAME DasmMot COMMAND dasmmot --seed 1)
add_test(NAME DasmMit COMMAND dasmmit --seed 1)
set_tests_properties(DasmMot DasmMit PROPERTIES LABELS "dasm")
