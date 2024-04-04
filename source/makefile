# ./source/makefile generated on Fri Dec 29 10:31:51 NZDT 2006 by root.
SHELL=/bin/sh
USR_BIN=/usr/local/bin
USR_LIB=/usr/local/lib
USR_INC=/usr/local/include
CC=gcc
CURSES_LIBS=-lcurses
CURSES_FLAG=-DHAVE_CURSES
TERMINFO_FLAG=-DHAVE_TERMINFO
HTTP_FLAG=-DHAVE_HTTP
REGEX_FLAG=-DHAVE_REGEX
IEEE_754_FLAG=-DHAVE_IEEE_754
STD_CFLAGS=-I$(USR_INC) -w 
STD_LD_FLAGS=-L$(USR_LIB) 
HS=algol68g.h config.h diagnostics.h environ.h genie.h gsl.h mp.h transput.h version.h
CS=acronym.o algol68g.o coercion.o complex.o diagnostics.o engine.o extensions.o generator.o genie.o gsl.o http.o io.o listing.o moids.o monitor.o mp.o names.o options.o parallel.o parser.o plotutils.o prelude.o regex.o scanner.o scope.o signal.o standard.o stowed.o support.o taxes.o torrix.o transput.o
FLAGS=$(CPPFLAGS) $(STD_CFLAGS) $(POSIX_THREADS_FLAG) $(PLOTUTILS_FLAG) $(GSL_FLAG) $(CURSES_FLAG) $(IEEE_754_FLAG) $(HTTP_FLAG) $(REGEX_FLAG) $(TERMINFO_FLAG) $(SQL_FLAG) $(CFLAGS)
all:    a68g
a68g:   $(CS)
	@echo "building a68g ..."
	@$(CC) $(CS) $(CFLAGS) $(STD_LD_FLAGS) $(POSIX_THREADS_LIBS) $(PLOTUTILS_LIBS) $(GSL_LIBS) $(CURSES_LIBS) $(SQL_LIBS) -lm $(LD_FLAGS) -o a68g
$(CS): $(HS)
.c.o:
	@echo "compiling $< ..."
	@$(CC) -c $(FLAGS) $<
clean:
	@rm -f $(CS)
depend:
	@touch $(HS)
