# ====================================================== #
# RogueBot — Makefile GNU (Hito 2 - Entregable 1+2)      #
# - Multidirectorio: descubre src/**.cpp automáticamente #
# - Paralelizable (make -jN) + dependencias .d           #
# - Benchmarks y empaquetado (.deb)                      #
# - Raylib estática por defecto (PREFER_RAYLIB_STATIC=1) #
# ====================================================== #

# --- Proyecto --- #
PROJECT      := roguebot
SRC_DIRS     := src
BUILD_DIR    := build_gnu
OBJ_DIR      := $(BUILD_DIR)/obj
BIN_DIR      := $(BUILD_DIR)/bin
TARGET       := $(BIN_DIR)/$(PROJECT)

UNAME_S := $(shell uname -s)

# --- Herramientas / ccache --- #
CXX ?= g++
ifdef CCACHE_BIN
CXX := ccache $(CXX)
endif

# --- Descubrimiento de fuentes --- #
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')
# Mapea: src/aaa/bbb.cpp -> build_gnu/obj/src/aaa/bbb.o
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# --- Flags de compilación/enlace --- #
CXXSTANDARD  ?= -std=gnu++17
OPTFLAGS     ?= -O2 -pipe
WARNFLAGS    ?= -Wall -Wextra -Wno-unknown-pragmas
DEPFLAGS     := -MMD -MP

# Incluye la raíz de src para headers en subcarpetas (core/, systems/, etc.)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INCLUDES := $(addprefix -I,$(INC_DIRS))

# Ruta de assets (parametrizable). En dev: "assets".
# En empaquetado Debian: ASSET_ROOT=/usr/share/roguebot/assets
ASSET_ROOT   ?= assets
DEFINES      := -DRB_ASSET_ROOT=\"$(ASSET_ROOT)\"

CXXFLAGS     := $(CXXSTANDARD) $(OPTFLAGS) $(WARNFLAGS) $(DEPFLAGS) $(DEFINES) $(INCLUDES)

# ========================= #
# Traducciones (GNU gettext) #
# ========================= #
LOCALES_DIR := assets/locales
I18N_SRCS   := $(shell find $(SRC_DIRS) -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \))
POT_FILE    := $(LOCALES_DIR)/$(PROJECT).pot

PO_FILES := \
  $(LOCALES_DIR)/es_ES/LC_MESSAGES/$(PROJECT).po \
  $(LOCALES_DIR)/en_GB/LC_MESSAGES/$(PROJECT).po

MO_FILES := $(PO_FILES:.po=.mo)
MERGED_PO_FILES := $(PO_FILES:.po=.po.merged)

# =============================== #
# raylib (estática por defecto)   #
# =============================== #
# Si quieres usar la raylib del sistema (pkg-config), ejecuta:
#    make PREFER_RAYLIB_STATIC=0
PREFER_RAYLIB_STATIC ?= 1

# Por defecto, sin extensión .exe (se ajusta más abajo para MinGW)
EXEEXT :=

ifeq ($(PREFER_RAYLIB_STATIC),1)
  # Forzar librería estática instalada en /usr/local por 'sudo make install' de raylib
  CXXFLAGS += -I/usr/local/include
  ifeq ($(UNAME_S),Linux)
    RAYLIB_LIBS := /usr/local/lib/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11
  endif
  ifeq ($(UNAME_S),Darwin)
    RAYLIB_LIBS := /usr/local/lib/libraylib.a -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework AudioToolbox
    LDFLAGS += -L/usr/local/lib
  endif
else
  # --- raylib (pkg-config si está; si no, fallbacks por plataforma) --- #
  RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
  RAYLIB_LIBS   := $(shell pkg-config --libs   raylib 2>/dev/null)

  ifeq ($(RAYLIB_LIBS),)
    ifeq ($(UNAME_S),Linux)
      RAYLIB_LIBS := -lraylib -lm -lpthread -ldl -lrt -lX11
    endif
    ifeq ($(UNAME_S),Darwin)
      RAYLIB_LIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework AudioToolbox
      RAYLIB_CFLAGS += -I/usr/local/include -I/opt/homebrew/include
      LDFLAGS += -L/usr/local/lib -L/opt/homebrew/lib
    endif
  endif

  # Añade flags de raylib si pkg-config los da
  CXXFLAGS += $(RAYLIB_CFLAGS)
endif

# Fallback Windows (MinGW/MSYS2)
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
  EXEEXT := .exe
  ifeq ($(PREFER_RAYLIB_STATIC),1)
    RAYLIB_LIBS := /usr/local/lib/libraylib.a -lopengl32 -lgdi32 -lwinmm
  else
    RAYLIB_LIBS += -lopengl32 -lgdi32 -lwinmm
  endif
endif

TARGET := $(BIN_DIR)/$(PROJECT)$(EXEEXT)

# ================== #
# Instalación / pkg  #
# ================== #
PREFIX       ?= /usr
DESTDIR      ?=
ASSETS_DIR   ?= assets
APP_DESKTOP  ?= packaging/roguebot.desktop
APP_ICON     ?= packaging/icons/roguebot.png

# ================== #
# Reglas principales #
# ================== #

.PHONY: all clean distclean run print-vars help \
        bench bench-nocache ccache-zero ccache-clear ccache-stats \
        install uninstall dist traducciones

all: $(TARGET)

$(TARGET): traducciones $(OBJS) | $(BIN_DIR)
	@echo "\033[1;34m [LINK]\033[0m $@"
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(RAYLIB_LIBS)

# Compilación de cada .cpp -> .o (crea carpeta espejo en obj/)
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "\033[1;32m [CXX ]\033[0m $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Directorios intermedios
$(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

# Limpiar
clean:
	@echo "\033[1;33m [CLEAN]\033[0m objetos"
	@rm -rf "$(OBJ_DIR)"

distclean:
	@echo "\033[1;33m [CLEAN]\033[0m todo build_gnu"
	@rm -rf "$(BUILD_DIR)"

# Ejecutar
run: $(TARGET)
	@echo "\033[1;36m [RUN ]\033[0m $(TARGET)"
	@$(TARGET)

# Ayuda / diagnóstico
print-vars:
	@echo "\033[1;36m=====================================\033[0m"
	@echo " 🤖 \033[1;36mRogueBot — Variables del entorno\033[0m"
	@echo "\033[1;36m=====================================\033[0m"
	@echo ""
	@echo "\033[1;33mPROJECT    \033[0m = \033[1;32m$(PROJECT)\033[0m"
	@echo "\033[1;33mCXX        \033[0m = \033[1;32m$(CXX)\033[0m"
	@echo "\033[1;33mCXXFLAGS   \033[0m = \033[0;37m$(CXXFLAGS)\033[0m"
	@echo "\033[1;33mLDFLAGS    \033[0m = \033[0;37m$(LDFLAGS)\033[0m"
	@echo "\033[1;33mRAYLIB_LIBS\033[0m = \033[0;37m$(RAYLIB_LIBS)\033[0m"
	@echo "\033[1;33mASSET_ROOT \033[0m = \033[0;37m$(ASSET_ROOT)\033[0m"
	@echo "\033[1;33mPREFER_RAYLIB_STATIC \033[0m = \033[1;32m$(PREFER_RAYLIB_STATIC)\033[0m"
	@echo "\033[1;33mSRCS (#)   \033[0m = \033[1;36m$(words $(SRCS))\033[0m"
	@echo "\033[1;33mOBJS (#)   \033[0m = \033[1;36m$(words $(OBJS))\033[0m"
	@echo ""

help:
	@echo ""
	@echo "\033[1;36m====================================\033[0m"
	@echo " 🤖 \033[1;36mRogueBot — Comandos disponibles\033[0m"
	@echo "\033[1;36m====================================\033[0m"
	@echo ""
	@echo "\033[1;32m make / make all\033[0m               	 -> compila todo"
	@echo "\033[1;32m make -jN\033[0m                      	 -> compila en paralelo (con N hilos)"
	@echo "\033[1;32m make -j\$$\(nproc\)\033[0m          	 -> usa automaticamente todos los hilos disponibles"
	@echo "\033[1;36m make run\033[0m                      	 -> ejecuta el binario"
	@echo "\033[1;33m make clean\033[0m                    	 -> limpia objetos"
	@echo "\033[1;33m make distclean\033[0m                	 -> limpia todo el build"
	@echo "\033[1;36m make print-vars\033[0m               	 -> muestra variables importantes"
	@echo "\033[1;32m make bench\033[0m                    	 -> mide tiempos con ccache"
	@echo "\033[1;32m make bench-nocache USE_CCACHE=0\033[0m -> mide tiempos sin ccache"
	@echo "\033[1;36m make ccache-zero\033[0m              	 -> pone a cero estadísticas"
	@echo "\033[1;36m make ccache-clear\033[0m             	 -> limpia la caché"
	@echo "\033[1;36m make ccache-stats\033[0m             	 -> muestra estadísticas"
	@echo "\033[1;35m make install [DESTDIR=staging]\033[0m  -> instala en árbol de empaquetado"
	@echo "\033[1;33m make uninstall\033[0m                  -> desinstala (útil en dev)"
	@echo "\033[1;35m make dist\033[0m                       -> genera paquete .deb en ./dist"
	@echo "\033[1;36m make traducciones\033[0m              -> genera .pot y compila .mo"
	@echo "\033[1;36m make actualizar-po\033[0m             -> actualiza .po desde el .pot"
	@echo ""


# ====================== #
# Traducciones (gettext) #
# ====================== #

traducciones: $(POT_FILE) actualizar-po $(MO_FILES)
	@echo "\033[1;35m [I18N]\033[0m .pot y .mo generados"

actualizar-po: $(POT_FILE)
	@echo "\033[1;35m [I18N]\033[0m Actualizando catálogos .po con msgmerge"
	@for po in $(PO_FILES); do \
	  if [ -f "$$po" ]; then \
	    msgmerge --update --backup=none "$$po" "$(POT_FILE)"; \
	  fi; \
	done

$(POT_FILE): $(I18N_SRCS)
	@echo "\033[1;35m [I18N]\033[0m Generando $@"
	@mkdir -p "$(LOCALES_DIR)"
	xgettext \
	  --language=C++ \
	  --from-code=UTF-8 \
	  --keyword=_ \
	  --keyword=N_ \
	  --output="$@" \
	  $(I18N_SRCS)

%.mo: %.po
	@echo "\033[1;35m [I18N]\033[0m Compilando $@"
	msgfmt -o "$@" "$<"

# ======================= #
# Benchmarks -jN y ccache #
# ======================= #

TIME_CMD := $(shell command -v /usr/bin/time 2>/dev/null || command -v gtime 2>/dev/null || echo time)
TIME_FMT := %E real, %U user, %S sys, CPU %P, Mem %M KB
NPROC    := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

bench-nocache: ; @for n in 1 2 3 4 8 12 16 ; do \
		echo "\033[1;34m== make -j$$n (nocache) ==\033[0m"; \
		CCACHE_DISABLE=1 $(MAKE) --no-print-directory distclean >/dev/null ; \
		CCACHE_DISABLE=1 $(TIME_CMD) -f "$(TIME_FMT)" $(MAKE) -j$$n --no-print-directory all >/devnull ; \
	done

bench:
	@echo "\033[1;34m== ccache: clear + zero ==\033[0m"
	@ccache -C || true
	@ccache -z || true
	@$(MAKE) --no-print-directory distclean >/dev/null
	@echo "\033[1;34m== CCACHE 1ª (poblar caché) -j$(NPROC) ==\033[0m"
	@$(TIME_CMD) -f "CCACHE 1ª | $(TIME_FMT)" $(MAKE) -j$(NPROC) --no-print-directory all >/dev/null
	@ccache -s || true
	@echo "\033[1;34m== CCACHE 2ª (cache hits) -j$(NPROC) ==\033[0m"
	@$(MAKE) --no-print-directory clean >/dev/null
	@$(TIME_CMD) -f "CCACHE 2ª | $(TIME_FMT)" $(MAKE) -j$(NPROC) --no-print-directory all >/dev/null
	@ccache -s || true

# ================== #
# Instalación system #
# ================== #
install: all
	@echo "\033[1;35m [INSTALL]\033[0m into $(DESTDIR)$(PREFIX)"
	# binario
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 0755 "$(TARGET)" "$(DESTDIR)$(PREFIX)/bin/roguebot"

	# assets del juego (usa '/.' para copiar también archivos ocultos)
	install -d "$(DESTDIR)$(PREFIX)/share/roguebot/assets"
	cp -r "$(ASSETS_DIR)/." "$(DESTDIR)$(PREFIX)/share/roguebot/assets/"

	# .desktop
	install -d "$(DESTDIR)$(PREFIX)/share/applications"
	install -m 0644 "$(APP_DESKTOP)" "$(DESTDIR)$(PREFIX)/share/applications/roguebot.desktop"

	# icono (256x256)
	install -d "$(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps"
	install -m 0644 "$(APP_ICON)" "$(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/roguebot.png"

# (Opcional) Desinstalación útil en dev (no usado por Debian)
uninstall:
	@echo "\033[1;33m [UNINSTALL]\033[0m from $(DESTDIR)$(PREFIX)"
	@rm -f  "$(DESTDIR)$(PREFIX)/bin/roguebot"
	@rm -f  "$(DESTDIR)$(PREFIX)/share/applications/roguebot.desktop"
	@rm -f  "$(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/roguebot.png"
	@rm -rf "$(DESTDIR)$(PREFIX)/share/roguebot"

# ====================== #
# Empaquetado (.deb)     #
# ====================== #
dist:
	@echo "\033[1;35m [DEB]\033[0m build package"
	@rm -rf dist && mkdir -p dist
	# Construcción del paquete binario sin firmar
	dpkg-buildpackage -us -uc -b
	# Mover artefactos al directorio dist
	@mkdir -p dist
	@mv ../*.deb dist/ 2>/dev/null || true
	@echo "\033[1;32m Paquete generado en ./dist \033[0m"

# Incluir dependencias autogeneradas (-MMD)
-include $(DEPS)
