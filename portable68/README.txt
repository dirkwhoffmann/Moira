v1.2

fixed word order of long write

fixed "link" instruction: prefetch was wrong

fixed stack frame creation for interrupts (changed "interrupt mask" is not stacked)

fixed interrupt handling: cpu doesn't memorize unserviced interrupts when all ipl lines negated

fixed exception cycle times

fixed "stop" instruction: disabled trace flag prevented stop state 

added callback for instruction logging

changed pure virtual method "getUserVector" in virtual. you have to override it only for USER_VECTOR interrupts (Amiga)

added check if TAS instruction performs indivisible read-modify-write cycle, so you can prevent other 68k instances from taking the bus too soon or prevent write in case of Genesis 1,2

cleaned and documented public interface

simplified stack frame creation for address/bus exceptions in case of INDIRECT ADDRESSING

TODO: check stack frame for program counter manipulating instructions in case of address/bus exceptions



v1.1

added unit tests for rte, rtr, trap, trapv

added public method to get last readed memory value from pc (reg irc)

added callback for reset instruction

added wordwrite and wordread callback (in some cases systems have to know if two bytes or one word was transfered, for example amiga register)

added a method to disable interrupt lines (interrupts will not be disabled after sampling like before)

added callback when cpu is in "halted" state (triggered by another bus/address error during bus/address error handling)

