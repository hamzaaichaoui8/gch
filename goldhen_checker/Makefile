TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
INTDIR      := build
LIBS        := -lc -lkernel -lSceNet -lSceSsl -lSceHttp -lSceSysmodule

CC          := clang
LD          := ld.lld
IDIRS       := -I$(TOOLCHAIN)/include
LDIRS       := -L$(TOOLCHAIN)/lib
CFLAGS      := -cc1 -triple x86_64-pc-freebsd-elf -munwind-tables $(IDIRS) -fuse-init-array -emit-obj
LFLAGS      := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr $(LDIRS) $(LIBS) $(TOOLCHAIN)/lib/crt1.o

CFILES      := $(wildcard src/*.c)
OBJS        := $(patsubst src/%.c, $(INTDIR)/%.o, $(CFILES))

TARGET      = goldhen_checker.oelf

_unused := $(shell mkdir -p $(INTDIR))

$(TARGET): $(INTDIR) $(OBJS)
	$(LD) $(INTDIR)/*.o -o $(INTDIR)/goldhen_checker.elf $(LFLAGS)
	$(TOOLCHAIN)/bin/linux/create-eboot -in=$(INTDIR)/goldhen_checker.elf -out=$(TARGET) --paid 0x3800000000000011

$(INTDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR):
	@mkdir $@

.PHONY: clean
clean:
	rm -rf $(INTDIR) $(TARGET)
