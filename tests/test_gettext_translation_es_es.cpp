#define BOOST_TEST_MODULE rb_test_gettext_translation_es_es
#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

#include <libintl.h>
#include <locale.h>

static uint32_t read32(const std::vector<unsigned char> &data, size_t off,
                       bool bigEndian) {
  if (off + 4 > data.size())
    return 0;
  if (bigEndian) {
    return (uint32_t(data[off]) << 24) | (uint32_t(data[off + 1]) << 16) |
           (uint32_t(data[off + 2]) << 8) | (uint32_t(data[off + 3]));
  }
  return (uint32_t(data[off])) | (uint32_t(data[off + 1]) << 8) |
         (uint32_t(data[off + 2]) << 16) | (uint32_t(data[off + 3]) << 24);
}

static bool pickPairFromMo(const char *moPath, std::string &outMsgid,
                           std::string &outMsgstr) {
  std::ifstream f(moPath, std::ios::binary);
  if (!f)
    return false;

  std::vector<unsigned char> data((std::istreambuf_iterator<char>(f)),
                                  std::istreambuf_iterator<char>());

  if (data.size() < 28)
    return false;

  const uint32_t MAGIC_LE = 0x950412de;
  const uint32_t MAGIC_BE = 0xde120495;

  uint32_t magicRaw = read32(data, 0, false);
  bool bigEndian = false;

  if (magicRaw == MAGIC_LE) {
    bigEndian = false;
  } else if (magicRaw == MAGIC_BE) {
    bigEndian = true;
  } else {
    magicRaw = read32(data, 0, true);
    if (magicRaw == MAGIC_LE)
      bigEndian = false;
    else if (magicRaw == MAGIC_BE)
      bigEndian = true;
    else
      return false;
  }

  const uint32_t nStrings = read32(data, 8, bigEndian);
  const uint32_t offOrig = read32(data, 12, bigEndian);
  const uint32_t offTrans = read32(data, 16, bigEndian);

  if (nStrings == 0)
    return false;
  if (offOrig + nStrings * 8 > data.size())
    return false;
  if (offTrans + nStrings * 8 > data.size())
    return false;

  bool foundAny = false;
  std::string bestId, bestStr;

  for (uint32_t i = 0; i < nStrings; ++i) {
    const size_t o = offOrig + i * 8;
    const size_t t = offTrans + i * 8;

    const uint32_t oLen = read32(data, o, bigEndian);
    const uint32_t oOff = read32(data, o + 4, bigEndian);
    const uint32_t tLen = read32(data, t, bigEndian);
    const uint32_t tOff = read32(data, t + 4, bigEndian);

    if (oOff + oLen > data.size())
      continue;
    if (tOff + tLen > data.size())
      continue;

    std::string msgid(reinterpret_cast<const char *>(&data[oOff]), oLen);
    std::string msgstr(reinterpret_cast<const char *>(&data[tOff]), tLen);

    if (msgid.empty())
      continue;

    if (msgid.find('\0') != std::string::npos)
      continue;

    const size_t nul = msgstr.find('\0');
    if (nul != std::string::npos)
      msgstr = msgstr.substr(0, nul);

    if (msgstr.empty())
      continue;

    if (!foundAny) {
      bestId = msgid;
      bestStr = msgstr;
      foundAny = true;
    }
    if (msgstr != msgid) {
      outMsgid = msgid;
      outMsgstr = msgstr;
      return true;
    }
  }

  if (!foundAny)
    return false;

  outMsgid = bestId;
  outMsgstr = bestStr;
  return true;
}

BOOST_AUTO_TEST_SUITE(Gettext_Integration_ES_ES)

BOOST_AUTO_TEST_CASE(translates_real_key_from_mo_es_es) {
  // Arrange
  const char *domain = "roguebot";
  const char *localeRoot = "assets/locales";
  const char *moPath = "assets/locales/es_ES/LC_MESSAGES/roguebot.mo";

  std::string msgid, expected;
  BOOST_REQUIRE_MESSAGE(
      pickPairFromMo(moPath, msgid, expected),
      "No se pudo leer/parsing del .mo o no hay entradas válidas: " << moPath);

  setenv("LANG", "es_ES.UTF-8", 1);
  setenv("LC_ALL", "es_ES.UTF-8", 1);
  setenv("LC_MESSAGES", "es_ES.UTF-8", 1);
  setenv("LANGUAGE", "es_ES", 1);

  BOOST_REQUIRE_MESSAGE(setlocale(LC_ALL, "") != nullptr,
                        "setlocale(LC_ALL, \"\") ha fallado. ¿Locales instalados?");

  bindtextdomain(domain, localeRoot);
  bind_textdomain_codeset(domain, "UTF-8");
  textdomain(domain);

  // Act
  const char *got = dgettext(domain, msgid.c_str());

  // Assert
  BOOST_REQUIRE_MESSAGE(got != nullptr, "gettext devolvió null");

  BOOST_TEST(std::string(got) == expected);
}

BOOST_AUTO_TEST_SUITE_END()
