ifndef CONFIG
	CONFIG=Release
endif

ifndef LIBDIR
	LIBDIR=/usr/lib/
endif

BUILD_DATE="$(shell date +'%Y %m %d %H %M')"

PAID := 1
PROJDIR := $(shell pwd)

VERSION := $(shell sh -c 'grep -oh -m 1 "VERSION=[0-9\.]*" standalone/builds/linux/Makefile | cut -d "=" -f 2')

MACHINE := $(shell sh -c 'uname -m 2> /dev/null || echo not')

WASMOPT :=
#FINAL_FLAGS := -sSHARED_MEMORY=1 -Wl,--shared-memory -O3 -g0 -sSIMD=1 -ftree-vectorize -flto -fvisibility=hidden -DNDEBUG=1 --closure 1 -sEVAL_CTORS -fno-rtti -fno-exceptions
EMXXFLAGS := -sINITIAL_MEMORY=642252800 -msimd128 -msse4.2 -sINVOKE_RUN=0 -sSTACK_SIZE=134217728 -sALLOW_MEMORY_GROWTH=0 --cache ./emsdk_cache -sUSE_WEBGL2=1 -sFULL_ES2=1 -sFULL_ES3=1 -sMIN_WEBGL_VERSION=2 -sMODULARIZE=1 -sEXPORT_NAME='createVial' -sEXPORTED_FUNCTIONS="['_release_lock','_acquire_lock','_clientAudioCallback','_setupAudioThread','_main','_dumpAudioBuffers','_setMacroValue','_setThreadMode','_vialSetWindowSize','_vialRedraw','_processKeyboardKey','_processMouseEvent','_processMidiEvent','_preinit','_processDnD','_malloc','_free','_vialLoadSlot0','_vialSaveSlot0','_setBPM','_setSamplerate']" -sEXPORTED_RUNTIME_METHODS="['HEAPU8','HEAPF32','HEAPU32','HEAP32','HEAPF64','FS','ccall']" -sLEGACY_VM_SUPPORT=0 -sIMPORTED_MEMORY=1 -sSHARED_MEMORY=1 -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=2 -sFORCE_FILESYSTEM -lidbfs.js

#DEBUG CONFIGS
#EMXXFLAGS += -O0 -g3 -ggdb -fvisibility=default -Wl,--keep-section=.debug_* "-DDEBUG=1" -gseparate-dwarf -gdwarf-4 -fno-split-dwarf-inlining --source-map-base http://localhost:3000/ -fdebug-prefix-map=/emsdk/=/emroot/ -sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2
#EMXXFLAGS += -sWASM=0 -sSEPARATE_DWARF=0

#-fsimd
#RELEASE CONFIGS
EMMXFLAGS += -O3 -flto -fvisibility=hidden "-DNDEBUG=1" --closure 1 -sEVAL_CTORS -fno-rtti -fno-exceptions -g0 -sSIMD=1 -sNO_EXIT_RUNTIME=1 --use-llvm-opts --disable-assertions -sSTACK_OVERFLOW_CHECK=1
EMMXFLAGS += -fopt-info-vec-optimized -fopt-info-vec-missed -sRETAIN_TYPE_INFO=1 -sOPTIMIZE_FOR_SIZE=0
# AGGRESSIVE MATH OPTIMISATIONS
EMMXFLAGS += -DFASTFUTILS=0 -ffast-math -fno-signed-zeros -fno-nans -fno-infs -fno-trapping-math -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-strict-aliasing -ftree-vectorize -finline-functions -funroll-loops -fvectorize -freorder-blocks -floop-vectorize

# LOGGING CHANGES
EMMXFLAGS += -Wno-nan-infinity -Wno-implicit-const-int-float-conversion -Wsimd --logging -Wunroll
#WASMOPT += --verbose 
#EMMXFLAGS += -g3 -ggdb

# RELAXED SIMD and other wasm features
EMMXFLAGS += -mrelaxed-simd
WASMOPT += --enable-relaxed-simd --enable-threads --enable-simd --enable-fp16 --disable-gc --disable-memory64

# wasm_hyprpass_extreme flags
XHYPRWASMOPT :=  -O4 --converge --const-hoisting --strip-debug
XHYPRWASMOPT +=  --debug
XHYPRWASMOPT +=  -uim -ffm -tnh -iit -ifwl -s 0 --optimize-level 9 --vacuum --type-ssa
XHYPRWASMOPT +=  --type-refining --type-merging --tuple-optimization --trap-mode-clamp
XHYPRWASMOPT +=  --ssa --simplify-locals --simplify-globals-optimizing --signature-pruning
XHYPRWASMOPT +=  --signature-refining --rse --roundtrip --flatten --rereloop --reorder-locals
XHYPRWASMOPT +=  --reorder-globals --reorder-functions --remove-unused-types --remove-unused-module-elements
XHYPRWASMOPT +=  --remove-unused-names --remove-unused-brs --precompute-propagate --post-emscripten
XHYPRWASMOPT +=  --optimize-instructions --optimize-casts --low-memory-unused --optimize-added-constants-propagate --once-reduction
XHYPRWASMOPT +=  --merge-locals --merge-similar-functions --merge-j2cl-itables --merge-blocks
XHYPRWASMOPT +=  --local-subtyping --licm --inlining-optimizing --heap-store-optimization
XHYPRWASMOPT +=  --gufa --gufa-optimizing --gto --gsi --global-refining --directize --dce
XHYPRWASMOPT +=  --dae-optimizing --const-hoisting --code-folding --code-pushing
XHYPRWASMOPT +=  --cfp --abstract-type-refining --monomorphize --pass-arg=monomorphize-min-benefit@30
XHYPRWASMOPT += --log-execution --simplify-locals --merge-locals --simplify-locals --coalesce-locals --merge-locals --vacuum

PROGRAM = vial
LIB_PROGRAM = Vial
LIB_PROGRAM_FX = VialFX
BIN = $(DESTDIR)/usr/bin
BINFILE = $(BIN)/$(PROGRAM)
LV2 = $(DESTDIR)/$(LIBDIR)/lv2/$(LIB_PROGRAM).lv2
EFFECTS_LV2 = $(DESTDIR)/$(LIBDIR)/lv2/$(LIB_PROGRAM_FX).lv2
VSTDIR = $(DESTDIR)/$(LIBDIR)/vst
VST = $(VSTDIR)/$(LIB_PROGRAM).so
VST3DIR = $(DESTDIR)/$(LIBDIR)/vst3
VST3 = $(VST3DIR)/$(LIB_PROGRAM).vst3
VST3SUBDIR = Contents/x86_64-linux
EFFECTS_VST = $(VSTDIR)/$(LIB_PROGRAM_FX).so
EFFECTS_VST3 = $(VST3DIR)/$(LIB_PROGRAM_FX).vst3
SYSDATA = $(DESTDIR)/usr/share/$(PROGRAM)
MAN = $(DESTDIR)/usr/share/man/man1/
CHANGES = $(DESTDIR)/usr/share/doc/$(PROGRAM)/
DESKTOP = $(DESTDIR)/usr/share/applications/
ZIP_FOLDER = $(LIB_PROGRAM)Binaries

ICONS      = $(DESTDIR)/usr/share/icons/hicolor/
ICON16     = images/vital_icon_16.png
ICON22     = images/vital_icon_22.png
ICON24     = images/vital_icon_24.png
ICON32     = images/vital_icon_32.png
ICON48     = images/vital_icon_48.png
ICON64     = images/vital_icon_64.png
ICON128    = images/vital_icon_128.png
ICON256    = images/vital_icon_256.png
XPMDEST    = $(DESTDIR)/usr/share/pixmaps
ICONXPM    = images/vital.xpm

ICONDEST16 = $(ICONS)/16x16/apps
ICONDEST22 = $(ICONS)/22x22/apps
ICONDEST24 = $(ICONS)/24x24/apps
ICONDEST32 = $(ICONS)/32x32/apps
ICONDEST48 = $(ICONS)/48x48/apps
ICONDEST64 = $(ICONS)/64x64/apps
ICONDEST128 = $(ICONS)/128x128/apps
ICONDEST256 = $(ICONS)/256x256/apps

all: standalone vst vst3 lv2

install_icons:
	install -d $(ICONDEST16) $(ICONDEST22) $(ICONDEST24) $(ICONDEST32)
	install -d $(ICONDEST48) $(ICONDEST64) $(ICONDEST128) $(ICONDEST256)
	install -d $(XPMDEST)
	cp $(ICONXPM) $(XPMDEST)/$(PROGRAM).xpm
	cp $(ICON16) $(ICONDEST16)/$(PROGRAM).png
	cp $(ICON22) $(ICONDEST22)/$(PROGRAM).png
	cp $(ICON24) $(ICONDEST24)/$(PROGRAM).png
	cp $(ICON32) $(ICONDEST32)/$(PROGRAM).png
	cp $(ICON48) $(ICONDEST48)/$(PROGRAM).png
	cp $(ICON64) $(ICONDEST64)/$(PROGRAM).png
	cp $(ICON128) $(ICONDEST128)/$(PROGRAM).png
	cp $(ICON256) $(ICONDEST256)/$(PROGRAM).png

standalone:
	$(MAKE) -C standalone/builds/linux CONFIG=$(CONFIG) EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

wasm_beta:
	$(MAKE) -C headless/builds/wasm CONFIG=$(CONFIG) EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

wasm_full:
	$(MAKE) -C standalone/builds/wasm_full CONFIG=$(CONFIG) EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1" PROJDIR=$(PROJDIR)

#--enable-relaxed-simd one day we will be free
#moderate time to optimise, great output
wasm_hyprpass_lite:
	@echo START HYPRPASS LITE
	@echo this can take a long time, please be patient! =O3
	-wasm-opt docs/vial.wasm -o docs/vialhypr.wasm -O3 -tnh --fast-math --enable-simd --enable-threads --simplify-locals --dce --vacuum --precompute --debug $(WASMOPT)
	@echo HYPRPASS COMPLETE!
	-sed "s/'vial.wasm'/'vialhypr.wasm'/g" docs/vial.js > docs/vial.tmp.js
	-mv docs/vial.tmp.js docs/vial.js

#very long time to optimise, getting kinda crazy atp
wasm_hyprpass_combo:
	@echo START HYPRPASS O4EXTREME
	@echo this can take a long time, please be patient! =O4
	-cp docs/vial.wasm docs/vialhypr.tmp.wasm
	-wasm-opt docs/vialhypr.tmp.wasm -o docs/vialhypr.tmp.wasm -O3 -tnh -iit --fast-math --simplify-locals --dce --vacuum --precompute --debug $(WASMOPT)
	-wasm-opt docs/vialhypr.tmp.wasm -o docs/vialhypr.tmp.wasm $(WASMOPT) --debug --precompute-propagate --type-refining-gufa --gufa-optimizing --optimize-instructions --optimize-casts --gufa --fast-math -tnh -iit --simplify-locals --merge-locals --reorder-globals --reorder-locals --remove-unused-brs
	-wasm-opt docs/vialhypr.tmp.wasm -o docs/vialhypr.tmp.wasm $(WASMOPT) --debug  --fast-math -tnh -iit --merge-blocks --coalesce-locals --optimize-instructions --vacuum --ssa --rse --remove-unused-brs --simplify-locals --merge-locals
	-wasm-opt docs/vialhypr.tmp.wasm -o docs/vialhypr.tmp.wasm $(WASMOPT) --debug  --fast-math -tnh -iit -O4 --converge --simplify-control-flow
	-wasm-opt docs/vialhypr.tmp.wasm -o docs/vialhypr.tmp.wasm $(WASMOPT) --debug  --fast-math -tnh -iit --flatten --rereloop --dfo
	-wasm-opt docs/vialhypr.tmp.wasm -o docs/vialhypr.tmp.wasm $(WASMOPT) --debug  --fast-math -tnh -iit -O4 --converge --simplify-control-flow
	-mv docs/vialhypr.tmp.wasm docs/vialhypr.wasm
	@echo HYPRPASS COMPLETE!
	-sed "s/'vial.wasm'/'vialhypr.wasm'/g" docs/vial.js > docs/vial.tmp.js
	-mv docs/vial.tmp.js docs/vial.js

#obtuse long time to optimise, literally cannot get more optimised than this.
wasm_hyprpass_dataflowssa:
	@echo START HYPRPASS O4DFO
	@echo this can take multiple weeks, please be patient! =O4
	-wasm-opt docs/vial.wasm -o docs/vialhypr.wasm $(WASMOPT) $(XHYPRWASMOPT) --flatten --dfo
	@echo HYPRPASS COMPLETE!
	-sed "s/'vial.wasm'/'vialhypr.wasm'/g" docs/vial.js > docs/vial.tmp.js
	-mv docs/vial.tmp.js docs/vial.js

#long time to optimise, near perfect output
wasm_hyprpass_convergent:
	@echo START HYPRPASS CONVERGENT
	@echo this can take a long time, please be patient! =O4conv
	-wasm-opt docs/vial.wasm -o docs/vialhypr.wasm -O4 -tnh --fast-math --enable-simd --enable-threads --simplify-locals --dce --vacuum --precompute --converge --debug $(WASMOPT)
	@echo HYPRPASS COMPLETE!
	-sed "s/'vial.wasm'/'vialhypr.wasm'/g" docs/vial.js > docs/vial.tmp.js
	-mv docs/vial.tmp.js docs/vial.js

lv2:
	$(MAKE) -C plugin/builds/linux_lv2 CONFIG=$(CONFIG) AR=gcc-ar EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

effects_lv2:
	$(MAKE) -C effects/builds/linux_lv2 CONFIG=$(CONFIG) AR=gcc-ar EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

vst:
	$(MAKE) -C plugin/builds/linux_vst VST CONFIG=$(CONFIG) AR=gcc-ar EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

vst3:
	$(MAKE) -C plugin/builds/linux_vst VST3 CONFIG=$(CONFIG) AR=gcc-ar EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

effects_vst:
	$(MAKE) -C effects/builds/linux_vst VST CONFIG=$(CONFIG) AR=gcc-ar EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

effects_vst3:
	$(MAKE) -C effects/builds/linux_vst VST3 CONFIG=$(CONFIG) AR=gcc-ar EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"v

headless_server:
	$(MAKE) -C headless/builds/linux CONFIG=$(CONFIG) EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

test:
	$(MAKE) -C tests/builds/linux CONFIG=$(CONFIG) EMXXFLAGS="$(EMXXFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE) CXXFLAGS="-DNO_AUTH=1"

clean:
	$(MAKE) clean -C headless/builds/was CONFIG=$(CONFIG)
	$(MAKE) clean -C plugin/builds/linux_vst CONFIG=$(CONFIG)
	$(MAKE) clean -C plugin/builds/linux_lv2 CONFIG=$(CONFIG)
	$(MAKE) clean -C effects/builds/linux_vst CONFIG=$(CONFIG)
	$(MAKE) clean -C effects/builds/linux_lv2 CONFIG=$(CONFIG)
	$(MAKE) clean -C headless/builds/linux CONFIG=$(CONFIG)
	$(MAKE) clean -C tests/builds/linux CONFIG=$(CONFIG)

install_standalone: standalone install_icons
	install -d $(BIN) $(MAN) $(CHANGES) $(DESKTOP)
	install standalone/builds/linux/build/$(PROGRAM) $(BIN)
	install -m644 standalone/vital.desktop $(DESKTOP)/vital.desktop

install_lv2: lv2
	install -d $(LV2)
	install -m644 plugin/builds/linux_lv2/Vital.lv2/* $(LV2)

install_effects_lv2: effects_lv2
	install -d $(EFFECTS_LV2)
	install -m644 effects/builds/linux_lv2/VitalFX.lv2/* $(EFFECTS_LV2)

install_vst: vst
	install -d $(VSTDIR)
	install plugin/builds/linux_vst/build/Vital.so $(VST)

install_effects_vst: effects_vst
	install -d $(VSTDIR)
	install effects/builds/linux_vst/build/VitalFX.so $(EFFECTS_VST)

install_vst3: vst3
	install -d $(VST3)/$(VST3SUBDIR)
	install -m644 plugin/builds/linux_vst/build/Vital.vst3/$(VST3SUBDIR)/* $(VST3)/$(VST3SUBDIR)

install_effects_vst3: effects_vst3
	install -d $(EFFECTS_VST3)/$(VST3SUBDIR)
	install -m644 plugin/builds/linux_vst/build/VitalFX.vst3/$(VST3SUBDIR)/* $(EFFECTS_VST3)/$(VST3SUBDIR)

install: install_standalone install_lv2 install_vst install_vst3
install_effects: install_effects_lv2 install_effects_vst install_effects_vst3

dist:
	rm -rf $(PROGRAM)
	mkdir $(PROGRAM)
	-cp -rf * $(PROGRAM)/
	$(MAKE) clean -C $(PROGRAM) CONFIG=Release
	$(MAKE) clean -C $(PROGRAM) CONFIG=Debug
	rm -rf $(PROGRAM)/.git
	rm -rf $(PROGRAM)/plugin/builds/CLion
	rm -rf $(PROGRAM)/plugin/builds/iOS
	rm -rf $(PROGRAM)/plugin/builds/osx
	rm -rf $(PROGRAM)/plugin/builds/vs17
	rm -rf $(PROGRAM)/plugin/builds/vs19
	rm -rf $(PROGRAM)/effects/builds/CLion
	rm -rf $(PROGRAM)/effects/builds/iOS
	rm -rf $(PROGRAM)/effects/builds/osx
	rm -rf $(PROGRAM)/effects/builds/vs17
	rm -rf $(PROGRAM)/effects/builds/vs19
	rm -rf $(PROGRAM)/standalone/builds/CLion
	rm -rf $(PROGRAM)/standalone/builds/iOS
	rm -rf $(PROGRAM)/standalone/builds/osx
	rm -rf $(PROGRAM)/standalone/builds/vs17
	rm -rf $(PROGRAM)/standalone/builds/vs19
	rm -rf $(PROGRAM)/tests/builds/CLion
	rm -rf $(PROGRAM)/tests/builds/iOS
	rm -rf $(PROGRAM)/tests/builds/osx
	rm -rf $(PROGRAM)/tests/builds/vs17
	rm -rf $(PROGRAM)/tests/builds/vs19
	mv $(PROGRAM) $(PROGRAM)-$(VERSION)
	tar -cvzf $(PROGRAM)_$(VERSION).orig.tar.gz $(PROGRAM)-$(VERSION)
	rm -rf $(PROGRAM)-$(VERSION)

zip_binaries:
	mkdir $(ZIP_FOLDER)
	cp -r plugin/builds/linux_lv2/Vital.lv2 $(ZIP_FOLDER)
	cp -r plugin/builds/linux_vst/build/Vital.so $(ZIP_FOLDER)
	cp -r plugin/builds/linux_vst/build/Vital.vst3 $(ZIP_FOLDER)
	cp -r standalone/builds/linux/build/$(PROGRAM) $(ZIP_FOLDER)
	zip -r $(ZIP_FOLDER) $(ZIP_FOLDER)

uninstall:
	rm -rf $(LV2)
	rm -rf $(VST)
	rm -rf $(SYSDATA)
	rm -rf $(BINFILE)
	rm $(ICONDEST16)/$(PROGRAM).png
	rm $(ICONDEST22)/$(PROGRAM).png
	rm $(ICONDEST24)/$(PROGRAM).png
	rm $(ICONDEST32)/$(PROGRAM).png
	rm $(ICONDEST48)/$(PROGRAM).png
	rm $(ICONDEST64)/$(PROGRAM).png
	rm $(ICONDEST128)/$(PROGRAM).png
	rm $(ICONDEST256)/$(PROGRAM).png
	rm $(XPMDEST)/$(PROGRAM).xpm

.PHONY: standalone
