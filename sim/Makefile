# MERLIN UI desktop simulator (host-only). macOS clang++ / any C++17 compiler.
#
#   make            build ./merlin-sim
#   make run        replay every script into out/
#   make check      nopollute + replay + pixel-exact diff against golden/
#   make golden     bless the current output as the new contract
#   make gallery    (re)write sim/gallery.html from golden/
#   make nopollute  fail if any Arduino header leaked into ui-merlin/

CXX      ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
UI       := ../examples/companion_radio/ui-merlin

UI_SRC := $(UI)/textutils.cpp $(UI)/glyphs.cpp $(UI)/icons.cpp $(UI)/router.cpp \
          $(UI)/channel.cpp $(UI)/sha256.cpp $(UI)/store.cpp \
          $(sort $(wildcard $(UI)/widgets/*.cpp)) \
          $(sort $(wildcard $(UI)/screens/*.cpp))
SIM_SRC := sim_gfx.cpp fixtures.cpp sim_main.cpp stb_impl.cpp
SRC     := $(UI_SRC) $(SIM_SRC)
HDR     := $(wildcard $(UI)/*.h) $(wildcard $(UI)/widgets/*.h) \
           $(wildcard $(UI)/screens/*.h) $(wildcard *.h)

BIN     := merlin-sim
SCRIPTS := $(sort $(wildcard scripts/*.txt))

.PHONY: all run check golden gallery nopollute clean

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run: $(BIN)
	@mkdir -p out
	@rm -f out/*.png
	@set -e; for s in $(SCRIPTS); do ./$(BIN) $$s out/; done
	@$(MAKE) --no-print-directory gallery

golden: run
	@mkdir -p golden
	@rm -f golden/*.png
	@cp out/*.png golden/
	@echo "blessed $$(ls golden/*.png | wc -l | tr -d ' ') golden frames"

# ui-merlin/ is platform-pure by construction, not by good intentions.
nopollute:
	@if grep -rn '#include <Arduino' $(UI) ; then \
	  echo "FAIL: Arduino header leaked into ui-merlin/"; exit 1; \
	fi
	@echo "nopollute: ui-merlin/ is Arduino-free"

check: nopollute run
	@fail=0; n=0; \
	for g in golden/*.png; do \
	  n=$$((n+1)); b=$$(basename $$g); \
	  if [ ! -f out/$$b ]; then echo "MISSING out/$$b"; fail=1; \
	  elif ! cmp -s $$g out/$$b; then echo "DIFF   $$b"; fail=1; fi; \
	done; \
	for o in out/*.png; do \
	  b=$$(basename $$o); \
	  if [ ! -f golden/$$b ]; then echo "UNBLESSED $$b"; fail=1; fi; \
	done; \
	if [ $$fail -ne 0 ]; then echo "check FAILED"; exit 1; fi; \
	echo "check OK: $$n frames pixel-identical to golden/"

gallery:
	@python3 gallery.py

clean:
	@rm -f $(BIN)
	@rm -rf out
