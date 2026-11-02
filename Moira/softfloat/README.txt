Berkeley SoftFloat Release 3e (John R. Hauser)
https://github.com/ucb-bar/berkeley-softfloat-3  (see COPYING.txt for the license)

Only the parts of SoftFloat that Moira needs are included: the 80-bit extended
format (extF80) with its conversions to and from 32-bit and 64-bit integers
and the 32-bit and 64-bit IEEE formats, plus the required primitives. The
"8086" specialization (x87 semantics for NaNs and invalid conversions) is used.

NOTE: This is a derivative work. Compared to the original distribution:

  - All files live in a single directory (headers, sources, 8086 specialization).
  - platform.h is a Moira-specific file (it replaces the build script options).
  - THREAD_LOCAL in softfloat.h is defined for C, C++ and MSVC.
  - moira_env.h / moira_env.c were added. They give C++ code access to the
    thread-local state of the library.
  - All external symbols are prefixed with "moira_" (e.g., moira_extF80_add) to
    avoid link-time clashes with other copies of SoftFloat in the same program.
  - Everything else is unmodified.

Use the class moira::Float80 (Moira/Float80.h) instead of calling these
functions directly.
