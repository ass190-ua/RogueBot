#pragma once

// Helpers para GNU gettext (i18n)
#include <libintl.h>

// Traduce en runtime
#ifndef _
#define _(String) gettext(String)
#endif

// Marca para traducir, pero NO traduce aún (útil para tablas estáticas)
#ifndef N_
#define N_(String) (String)
#endif
