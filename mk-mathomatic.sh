# run gcc compiler in freestanding mode
gcc -g -Os -static -fno-pie -no-pie -nostdlib -nostdinc \
  -fno-omit-frame-pointer -pg -mnop-mcount -mno-tls-direct-seg-refs \
  -o mathomatic.com.dbg mathomatic-am.c \
  -DUNIX -DVERSION=\"16.0.5\" \
  -DWITH_COSMOPOLITAN \
  -Wl,--gc-sections -fuse-ld=bfd -Wl,--gc-sections \
  -Wl,-T,ape.lds -include cosmopolitan.h crt.o ape-no-modify-self.o cosmopolitan.a
objcopy -S -O binary mathomatic.com.dbg mathomatic.com


# NOTE: scp it to windows/mac/etc. *before* you run it!
# ~40kb static binary (can be ~16kb w/ MODE=tiny)
./ape.elf ./mathomatic.com
#  -DSTACK_FRAME_UNLIMITED \
#  -fno-gcse -ffunction-sections -fdata-sections \
