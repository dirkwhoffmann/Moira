// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* UAE's CPU core (newcpu.cpp and the generated cpuemu_*.cpp files) is
 * entangled with the rest of the emulator (custom chips, debugger, savestates,
 * JIT, GUI, ...). Only the CPU is needed here, so this file provides the
 * remaining symbols as inert stand-ins. None of them is expected to do any
 * work when a single instruction is executed with a flat memory model.
 */

#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "events.h"
#include "gui.h"
#include "ar.h"
#include "debug.h"
#include "disasm.h"
#include "inputrecord.h"
#include "savestate.h"
#include "autoconf.h"
#include "uae.h"
#include "cpuboard.h"
#include "threaddep/thread.h"
#include "uae/vm.h"

//
// Global state
//

struct uae_prefs currprefs, changed_prefs;
struct gui_info gui_data;
struct ev eventtab[ev_max];
struct cpum2c m2cregs[1] = {};

int config_changed, config_changed_flags;
int debugging, exception_debugging, debug_dma;
bool debug_opcode_watch, debugmem_trace, event_wait;
int input_record, input_play, inputrecord_debug;
int quit_program;
int savestate_state;
TCHAR savestate_fname[MAX_DPATH];
int special_mem, special_mem_default;
bool cloanto_rom;
int uae_boot_rom_type;
uaecptr rtarea_base;
uaecptr wait_for_pc;
int action_replay_flag, hrtmon_flag;
uae_u32 hrtmem_start, hrtmem_size;
volatile uae_atomic uae_int_requested;
uae_u32 hsync_counter, vsync_counter;
uae_u16 dmacon, intreq;
int vpos, maxvpos;
frame_time_t vsyncmintimepre, vsyncmaxtime, vsynctimebase;
evt_t start_cycles, currcycle, currcycle_cck;
uae_u8 ce_banktype[65536], ce_cachable[65536];
uae_u32 start_pc;
uae_u8 *start_pc_p;
void *pushall_call_handler;
void fp_init_native(void) { }

//
// Functions
//

void write_log(const char *, ...) { }
void console_out_f(const char *, ...) { }
void gui_message(const char *, ...) { }
void gui_led(int, int, int) { }
void notify_user(int) { }
void uae_restart(struct uae_prefs *, int, const char *) { }
void uae_reset(int, int) { }
int sleep_millis(int) { return 0; }
int sleep_millis_main(int) { return 0; }
bool is_mainthread() { return true; }
void warpmode(int) { }
uae_u32 uaerand() { return 0; }
void uaerandomizeseed() { }
uae_u32 uaerandgetseed() { return 0; }
uae_u32 uaesetrandseed(uae_u32) { return 0; }
uae_time_t uae_time() { return 0; }

void INTREQ_INT(int, int) { }
bool INTREQ_0(uae_u16) { return false; }
void INTREQ_f(uae_u16) { }
int intlev() { return 0; }
void intlev_ack(int) { }
void init_custom() { }
void custom_reset(bool, bool) { }
void custom_prepare() { }
void custom_cpuchange() { }
void custom_end_drawing() { }
void blitnasty() { }
void clear_events() { }
void events_schedule() { }
void send_internalevent(int) { }
void event2_newevent_xx(int, evt_t, uae_u32, evfunc2) { }
void audio_deactivate() { }
void statusline_clear() { }
void reset_frame_rate_hack() { }
void a3000_fakekick(int) { }
void check_prefs_changed_audio() { }
void set_config_changed(int) { }
void target_cpu_speed() { }
void fixup_cpu(struct uae_prefs *) { }
uaecptr need_uae_boot_rom(struct uae_prefs *) { return 0; }
void mman_set_barriers(bool) { }
void uae_vm_jit_write_protect(bool) { }
void device_call_main_thread_callbacks() { }
void cpu_thread_flush_register_batch() { }
void set_cache_state(int) { }
int check_for_cache_miss() { return 0; }
void expansion_cpu_fallback() { }
void protect_roms(bool) { }
void memory_clear() { }
void restore_banks() { }
void memory_restore() { }
int memory_valid_address(uaecptr, uae_u32) { return 1; }

// Debugger and tracing
void debug() { }
void activate_debugger() { }
void debug_smc_clear(uaecptr, int) { }
void debug_trainer_match() { }
void debug_init_trainer(const TCHAR *) { }
void debugmem_flushcache(uaecptr, int) { }
bool debugmem_illg(uae_u16) { return false; }
bool debug_get_prefetch(int, uae_u16 *) { return false; }
void branch_stack_push(uaecptr, uaecptr) { }
void branch_stack_pop_rts(uaecptr) { }
void branch_stack_pop_rte(uaecptr) { }
void memory_map_dump() { }
void record_dma_reset(int) { }
void record_dma_event(uae_u32) { }
void record_dma_event_data(uae_u32, uae_u32) { }
void record_dma_ipl() { }
void log_dma_record() { }
int inprec_open(const TCHAR *, const TCHAR *) { return 0; }
void inprec_startup() { }
bool inprec_prepare_record(const TCHAR *) { return false; }
void inprec_playdebug_cpu(int, uae_u16) { }
void inprec_recorddebug_cpu(int, uae_u16) { }

// Special hardware
void action_replay_hide() { }
void action_replay_enter() { }
void hrtmon_hide() { }
void hrtmon_enter() { }
void hrtmon_breakenter() { }
int is_ar_pc_in_rom() { return 0; }
int is_ar_pc_in_ram() { return 0; }
void fpux_restore(int *) {}

// Savestates
void savestate_init() { }
bool savestate_check() { return false; }
void savestate_rewind() { }
void savestate_restore_final() { }
bool savestate_restore_finish() { return false; }
uae_u32 get_statefile_version() { return 0; }
void restore_state(const TCHAR *) { }
void save_u8_func(uae_u8 **, uae_u8) { }
void save_u16_func(uae_u8 **, uae_u16) { }
void save_u32_func(uae_u8 **, uae_u32) { }
void save_u64_func(uae_u8 **, uae_u64) { }
uae_u8 restore_u8_func(uae_u8 **) { return 0; }
uae_u16 restore_u16_func(uae_u8 **) { return 0; }
uae_u32 restore_u32_func(uae_u8 **) { return 0; }
uae_u64 restore_u64_func(uae_u8 **) { return 0; }

// Threads
int uae_sem_init(uae_sem_t *, int, int) { return 0; }
int uae_start_thread(const char *, uae_thread_function, void *, uae_thread_id *) { return 0; }
int uae_wait_thread(uae_thread_id *) { return 0; }
SDL_ThreadID uae_thread_get_id(SDL_Thread *) { return 0; }

// Disassembler and JIT
uae_u32 m68k_disasm_2(TCHAR *, int, uaecptr, uae_u16 *, int, uaecptr *, int, uae_u32 *, uae_u32 *, uaecptr, int) { return 0; }
#if defined(JIT) && defined(CPU_AARCH64)
void build_comp() { }
void compile_block(cpu_history *, int, int) { }
#endif

// Memory access with side effects
bool cpuboard_fc_check(uae_u32, uae_u32 *, int, bool) { return false; }
uaecptr cpuboard_get_reset_pc(uaecptr *) { return 0; }
bool cpuboard_forced_hardreset() { return false; }
