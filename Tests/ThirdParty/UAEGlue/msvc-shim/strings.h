// MSVC has no <strings.h>. UAE includes it for the case-insensitive string functions.
#pragma once
#include <string.h>
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
