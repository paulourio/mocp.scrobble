
GPP=g++
INCLUDE=-I/usr/include/lastfmlib
LIBS=-llastfmlib
OUTPUT=run

scrobble:
	$(GPP) scrobble.cpp $(INCLUDE) $(LIBS) -o $(OUTPUT)

default: scrobble

clean:
	rm $(OUTPUT)
