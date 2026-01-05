#pragma once

#if defined(RB_ENABLE_I18N) && RB_ENABLE_I18N
#include <libintl.h>
#else
inline const char *bindtextdomain(const char *, const char *) {
  return nullptr;
}
inline const char *bind_textdomain_codeset(const char *, const char *) {
  return nullptr;
}
inline const char *textdomain(const char *) { return nullptr; }
inline const char *gettext(const char *s) { return s; }
#endif
