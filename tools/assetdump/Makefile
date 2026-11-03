# Build and run on the Mac: make -C tools/assetdump run
UP = ../../upstream
SRC = assetdump.c $(UP)/third_party/cgenius/fileio/lz.c $(UP)/third_party/cgenius/fileio/compression/Cunlzexe.c

assetdump: $(SRC)
	cc -O2 -std=c99 -I$(UP) -o $@ $(SRC)

run: assetdump
	./assetdump ../../data/KEEN1 ../../assets

.PHONY: run
