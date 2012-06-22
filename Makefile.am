AM_CPPFLAGS := -DSTRICT -D_UNICODE -D_WIN32_WINNT=0x0600
AM_CXXFLAGS := -Wall -Wextra -std=c++0x -municode -mwindows

noinst_PROGRAMS := qarma

qarma_LDADD := -lws2_32 -lcomctl32
qarma_SOURCES := explain.cpp main_window.cpp master_proto.cpp qarma.cpp query_proto.cpp querymanager.cpp qarma-rc.rc enctypex_decoder.c

qarma-rc.o: qarma.manifest

.rc.o:
	$(AM_V_GEN)
	$(AM_V_at)$(WINDRES) -o $@ $<
