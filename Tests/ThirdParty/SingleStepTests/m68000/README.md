# m68000
Repository for TomHarte-style JSON tests for the Motorola 68000. Generated using the microcoded core in MAME.

STATUS: all of the tests except TAS and TRAPV are verified as good.

Caveats:

* There's a new cycle type in addition to idle, read, write, and TAS. "re" for read address error, and "we" for write address error. On real m68k, they still happen, AS just isn't asserted, so the results aren't committed. The transactions are left in here with the new type to simplify correctly catching address errors 
* TAS doesn't properly handle the special 5-cycle TAS read-modify-write timing.
* There's some strange issue I don't understand with the TRAPV tests. Or maybe I'm just interpreting them wrong. It appears to trigger incorrectly based on the S bit?
* Any bugs that exist in Mame's microcoded M68000 emulator will exist here too

Use decode.py to convert from .json.bin to .json.

They are in ALMOST the same format as the TomHarte tests, just generated with a better emulator, and:

* RAM pieces are now in 16 bits, as it is on the real processor.
* There are the new "re" and "we" cycle types as mentioned above
* PC is now set using m_au from MAME. To clarify, it has a number of registers, m_pc, m_ipc, m_au, all of which work as a sort of PC, but are updated differently. m_au seems to be consistent though. It's "next prefetch address" so it's +4 from where the test starts executing.
* Data bus now always is as real processor (i.e. only UDS is on, and you read 0xAB, you will get 0xAB00 for data bus. This differs from TomHarte where it would would give 0xAB)
* The tests now include UDS and LDS in the transaction logs, since the real M68K can't output A0. 

These may not be the final form; I may add new features, or adjust so that certain things go better, etc., but they're worth using now.

Thanks to the MAME project for the awesome microcoded emulator! Thanks to TomHarte for the idea for the JSON tests!