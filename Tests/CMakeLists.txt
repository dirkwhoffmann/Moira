# Tests and test tools
#
#   Unit/        unit                        doctest unit tests
#   Smoke/       smoke                       SingleStepTests, a few seconds
#   Random/      randomuae randommusashi randommame  randomized differential tests
#   Exception/   exceptionuae exceptionmame  exceptions in the middle of instructions
#   Dasm/        dasm dasmmusashi            disassembler tests

add_subdirectory(ThirdParty)
add_subdirectory(Unit)
add_subdirectory(Smoke)
add_subdirectory(Random)
add_subdirectory(Exception)
add_subdirectory(Dasm)
