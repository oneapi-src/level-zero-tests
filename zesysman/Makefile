ZEPREFIX = /usr/local

ZEINCLUDE = $(ZEPREFIX)/include/level_zero
ZELIB = $(ZEPREFIX)/lib

SYSMAN_MODULE = zesysman.py
SYSMAN_PROG = build/zesysman/zesysman
SETUP_PROG = ./setup.py

BUILD_EXT_FLAGS = -I $(ZEINCLUDE) -L $(ZELIB) -lze_loader --swig-opts -I$(ZEINCLUDE)
INSTALL_FLAGS = --prefix build/zesysman --install-platlib build/zesysman --install-scripts build/zesysman

INTERMEDIATE_SOURCE = zesysman_wrap.c
EXECUTION_ARTIFACTS = zesysman.pyc build __pycache__

CSCOPE_FILES = cscope.out cscope.out.in cscope.out.po

.PHONY: run cscope clean clobber

run : $(SYSMAN_MODULE)
	LD_LIBRARY_PATH=$(ZELIB) $(SYSMAN_PROG) --device list

$(SYSMAN_MODULE) : zesysman.i setup.py zesysman
	$(SETUP_PROG) build_ext $(BUILD_EXT_FLAGS) build_scripts install $(INSTALL_FLAGS)

cscope : $(CSCOPE_FILES)

$(CSCOPE_FILES) :
	cscope -R -b -q -f cscope.out -I $(ZEINCLUDE)

clean :
	$(RM) $(CSCOPE_FILES) $(INTERMEDIATE_SOURCE)
	$(RM) -r $(EXECUTION_ARTIFACTS)

clobber : clean
	$(RM) $(SYSMAN_MODULE) *.so
