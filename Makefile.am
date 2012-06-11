AM_CXXFLAGS := -Wall -Wextra

noinst_PROGRAMS := qarma

qarma_CXXFLAGS := -municode -std=c++0x
qarma_LDADD := -lws2_32 -lgdi32 -lcomctl32
qarma_LDFLAGS := -Wl,--subsystem=windows
qarma_SOURCES := qarma.cpp qarma-rc.rc

qarma-rc.o: qarma.manifest

.rc.o:
	$(AM_V_GEN)
	$(AM_V_at)$(WINDRES) -o $@ $<
