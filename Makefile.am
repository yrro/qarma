AM_CXXFLAGS := -Wall -Wextra -std=c++0x -municode -mwindows

noinst_PROGRAMS := qarma

qarma_LDADD := -lws2_32 -lcomctl32
qarma_SOURCES := qarma.cpp qarma-rc.rc

qarma-rc.o: qarma.manifest

.rc.o:
	$(AM_V_GEN)
	$(AM_V_at)$(WINDRES) -o $@ $<
