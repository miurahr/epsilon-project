bin_PROGRAMS = epsilon
epsilon_SOURCES = epsilon.c pbm.c cmd_version.c cmd_list_all_fb.c \
	cmd_encode_file.c psi.c cmd_decode_file.c misc.c cmd_truncate_file.c cmd_start_node.c \
	worker_mpi_node.c

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/src $(all_includes)

# the library search path.
epsilon_LDFLAGS = $(all_libraries) 
epsilon_LDADD = $(top_builddir)/lib/libepsilon.la
noinst_HEADERS = pbm.h options.h cmd_version.h cmd_list_all_fb.h \
	cmd_encode_file.h psi.h misc.h cmd_decode_file.h cmd_truncate_file.h cmd_start_node.h \
    worker_mpi_node.h epsilon_version.h
