add_library(moira STATIC

Moira.cpp
MoiraDebugger.cpp
)

target_compile_options(moira PUBLIC -Wno-unused-parameter)
target_compile_options(moira PUBLIC -Wno-unused-but-set-parameter)
target_compile_options(moira PUBLIC -Wno-unused-but-set-variable)
target_compile_options(moira PUBLIC -Wno-missing-field-initializers)

if(MSVC)

  target_compile_options(vAmigaCore PUBLIC /W4 /WX /bigobj)
  target_compile_options(vAmigaCore PUBLIC /wd4100 /wd4201 /wd4324 /wd4458)
  set_source_files_properties(Moira/Moira.cpp PROPERTIES COMPILE_FLAGS "/wd4127")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

  target_compile_options(vAmigaCore PUBLIC -Wall -Werror)
  target_compile_options(vAmigaCore PUBLIC -Wno-unused-parameter)
  target_compile_options(vAmigaCore PUBLIC -fconcepts)
  target_compile_options(vAmigaCore PUBLIC -Wno-restrict)
  target_compile_options(vAmigaCore PUBLIC -Wno-dangling-pointer)

else()

  target_compile_options(vAmigaCore PUBLIC -Wall -Werror)
  target_compile_options(vAmigaCore PUBLIC -Wno-unused-parameter)

endif()
