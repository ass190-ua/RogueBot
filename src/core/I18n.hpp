#pragma once

#if defined(RB_ENABLE_I18N) && RB_ENABLE_I18N
#include <libintl.h>

#ifndef _
#define _(String) gettext(String)
#endif
#else
// i18n desactivado: compilar sin gettext
#ifndef _
#define _(String) (String)
#endif
#endif

#ifndef N_
#define N_(String) (String)
#endif
