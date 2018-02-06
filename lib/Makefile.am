INCLUDES = -I$(top_srcdir)/lib
METASOURCES = AUTO
libepsilon_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
lib_LTLIBRARIES = libepsilon.la
libepsilon_la_SOURCES = bit_io.c checksum.c cobs.c color.c common.c dc_level.c \
	filter.c filterbank.c libmain.c list.c mem_alloc.c merge_split.c pad.c \
	resample.c speck.c
noinst_HEADERS = bit_io.h checksum.h cobs.h color.h common.h daub97lift.h \
	dc_level.h filter.h filterbank.h libmain.h list.h mem_alloc.h merge_split.h pad.h \
	resample.h speck.h msvc/inttypes.h msvc/stdint.h
include_HEADERS = epsilon.h 
