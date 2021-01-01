.phony all:
all: disk

disk:
	gcc diskinfo.c -o diskinfo
	gcc disklist.c -o disklist
	gcc diskget.c -o diskget
	gcc diskput.c -o diskput

.PHONY clean:
clean:
	-rm -rf *.o *.exe
