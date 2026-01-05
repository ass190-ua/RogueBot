#pragma once

// Si CMake define RB_ENABLE_I18N, obedecemos.
#if defined(RB_ENABLE_I18N)

#if RB_ENABLE_I18N
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#ifndef _
#define _(String) (String)
#endif
#endif

// Si NO está definido (caso Makefile), autodetectamos.
#else

#if defined(__has_include)
#if __has_include(<libintl.h>)
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#ifndef _
#define _(String) (String)
#endif
#endif
#else
#ifndef _
#define _(String) (String)
#endif
#endif

#endif

#ifndef N_
#define N_(String) (String)
#endif
