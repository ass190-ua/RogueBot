#pragma once

// Caso 1: CMake define RB_ENABLE_I18N explícitamente
#if defined(RB_ENABLE_I18N)

#if RB_ENABLE_I18N
#include <libintl.h>
#else
extern "C" {
inline char *bindtextdomain(const char *, const char *) { return nullptr; }
inline char *bind_textdomain_codeset(const char *, const char *) {
  return nullptr;
}
inline char *textdomain(const char *) { return nullptr; }
inline char *gettext(const char *s) { return const_cast<char *>(s); }
}
#endif

// Caso 2: RB_ENABLE_I18N NO está definido (ej. Makefile local).
// Si el sistema tiene libintl.h (Linux), lo usamos.
// Si no lo tiene (Windows sin gettext), usamos stubs.
#else

#if defined(__has_include)
#if __has_include(<libintl.h>)
#include <libintl.h>
#else
extern "C" {
inline char *bindtextdomain(const char *, const char *) { return nullptr; }
inline char *bind_textdomain_codeset(const char *, const char *) {
  return nullptr;
}
inline char *textdomain(const char *) { return nullptr; }
inline char *gettext(const char *s) { return const_cast<char *>(s); }
}
#endif
#else
// Compiladores sin __has_include: asumimos que no hay gettext
extern "C" {
inline char *bindtextdomain(const char *, const char *) { return nullptr; }
inline char *bind_textdomain_codeset(const char *, const char *) {
  return nullptr;
}
inline char *textdomain(const char *) { return nullptr; }
inline char *gettext(const char *s) { return const_cast<char *>(s); }
}
#endif

#endif
