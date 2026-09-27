SHARED_FILES=src/fs.c src/path.c src/global.c

all:
	mkdir -p output
	
	gcc src/mkfs.c $(SHARED_FILES) -o output/mkfs.neofs -v
	gcc src/readfs.c $(SHARED_FILES) -o output/read.neofs
	gcc src/copyfs.c $(SHARED_FILES) -o output/copy.neofs
	gcc src/removefs.c $(SHARED_FILES) -o output/remove.neofs

clean:
	rm -f output/*