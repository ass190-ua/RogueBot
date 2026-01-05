#pragma once

// Si i18n está activo, usamos gettext real.
// Si NO, dejamos macros “stub” para que el código compile.
#if defined(RB_ENABLE_I18N) && RB_ENABLE_I18N
#include <libintl.h>

#ifndef _
#define _(String) gettext(String)
#endif
#else
#ifndef _
#define _(String) (String)
#endif
#endif

#ifndef N_
#define N_(String) (String)
#endif
