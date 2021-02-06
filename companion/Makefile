LIBS = -limgui -lvitagl -lSceAppmgr_stub -lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lm -lc -lSceCommonDialog_stub \
	-lSceTouch_stub -lmathneon -lvitashark -lSceShaccCg_stub

COMMON_OBJS = main.o

CFILES		:=	$(COMMON_OBJS)
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 
			
PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O3
CXXFLAGS  = $(CFLAGS) -fno-exceptions -Wno-write-strings
ASFLAGS = $(CFLAGS)

all: eboot.bin

eboot.bin: eboot.velf
	vita-make-fself -s $< eboot.bin
	
%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

eboot.elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf eboot.velf eboot.elf $(OBJS) eboot.bin