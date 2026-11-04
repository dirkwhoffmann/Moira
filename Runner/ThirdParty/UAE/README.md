# UAE CPU core (test runner reference)

This directory contains the 68k CPU core of the UAE Amiga emulator. The test
runner uses it as an alternative reference implementation next to Musashi
(see `MOIRA_MIMIC_UAE` in `Moira/MoiraConfig.h`).

## Provenance

* **CPU core: WinUAE** (v6.1.0, `WinUAE-master`, fetched 2026-11-04), unmodified
  except for the build adaptations listed below:
  `cpuemu_*.cpp`, `cpudefs.cpp`, `cpustbl.cpp`, `cputbl.h`, `readcpu.*`,
  `newcpu.cpp`, `newcpu_common.cpp`, `cpummu*.cpp`, `fpp*.cpp`, `softfloat/*`,
  and the headers `include/{newcpu,fpp,cpummu,cpummu030,readcpu,mmu_common}.h`.
* **Host environment: Amiberry.** WinUAE is tied to Windows. The remaining
  headers (`sysdeps.h`, `options.h`, `osdep/`, `threaddep/`, ...) are taken from
  Amiberry, UAE's portable fork, which provides the platform layer. Amiberry's
  own CPU files are no longer used. (Licenses: `LICENSE-WinUAE`, GPLv2 or later;
  `LICENSE-Amiberry`, GPLv3.)

The glue code that drives the core lives in `../UAEGlue`.

## Adaptations to the WinUAE files

* `cpuemu_0/11/13/40/50.cpp`, `newcpu.cpp`: WinUAE's x86 flag layout
  (`regflags.cznv`) is guarded with `#if defined(CPU_i386) || defined(CPU_x86_64)`,
  with an `nzcv` variant for ARM (the same patch Amiberry applies).
* `include/newcpu.h`: includes `thread.h` for `atomic_or()` & co.
* `osdep/sysconfig.h`: `WITH_THREADED_CPU` can be disabled with
  `AMIBERRY_NO_THREADED_CPU` (WinUAE's `newcpu.cpp` has its own threaded CPU
  that is incompatible with Amiberry's).
* `osdep/target.h`: `typedef unsigned long ULONG` (used by `cpummu.cpp`).

When updating from a newer WinUAE, copy the files listed above and reapply
these changes.
