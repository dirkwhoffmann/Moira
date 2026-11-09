// Minimal stand-in for <SDL3/SDL.h>.
//
// UAE (Amiberry flavour) uses SDL for portable string helpers and thread types.
// The CPU core needs none of SDL's functionality, so this header maps the few
// symbols that the compiled UAE sources reference to the C library. It makes
// the test runner independent of an installed SDL.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>

#define SDL_isdigit        isdigit
#define SDL_isspace        isspace
#define SDL_tolower        tolower
#define SDL_toupper        toupper
#define SDL_atof           atof
#define SDL_atoi           atoi
#define SDL_snprintf       snprintf
#define SDL_vsnprintf      vsnprintf
#define SDL_strtod         strtod
#define SDL_strtol         strtol
#define SDL_strtoul        strtoul
#define SDL_strtoll        strtoll
#define SDL_strchr         strchr
#define SDL_strrchr        strrchr
#define SDL_strstr         strstr
#define SDL_strcmp         strcmp
#define SDL_strncmp        strncmp
#define SDL_strcasecmp     strcasecmp
#define SDL_strncasecmp    strncasecmp
#define SDL_strdup         strdup
#define SDL_strlen         strlen

// Used by the abort macro in sysdeps.h
#define SDL_Quit()         ((void)0)

// Opaque types referenced by declarations only
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Semaphore SDL_Semaphore;
typedef struct SDL_Thread SDL_Thread;
typedef unsigned long SDL_ThreadID;
typedef int (*SDL_ThreadFunction)(void *);
typedef int SDL_GamepadButton;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;

// Semaphores: referenced by inline helpers of the UAE headers, never called by the CPU core
static inline void SDL_SignalSemaphore(SDL_Semaphore *) { }
static inline void SDL_WaitSemaphore(SDL_Semaphore *) { }
