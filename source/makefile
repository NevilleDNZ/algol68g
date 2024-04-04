# ./source/makefile generated on Mon May  5 08:56:18 EST 2008 by ndempsey.
SHELL=/bin/sh
USR_BIN=/home/ndempsey/prj/a68/algol68g-mk12
USR_LIB=/usr/local/lib
USR_INC=/usr/local/include
CC=gcc
POSIX_THREADS_LIBS=-lpthread
POSIX_THREADS_FLAG=-DENABLE_PAR_CLAUSE
PLOTUTILS_LIBS=-L/usr/X11R6/lib -lXaw -lXmu -lXt -lSM -lICE -lXext -lX11 -lplot
PLOTUTILS_FLAG=-DENABLE_GRAPHICS
GSL_LIBS=-lgsl -lgslcblas
GSL_FLAG=-DENABLE_NUMERICAL
CURSES_LIBS=-lcurses
CURSES_FLAG=-DENABLE_CURSES
TERMINFO_FLAG=-DENABLE_TERMINFO
HTTP_FLAG=-DENABLE_HTTP
REGEX_FLAG=-DENABLE_REGEX
SQL_DIR=/usr
SQL_LIBS=-lpq
SQL_FLAG=-DENABLE_POSTGRESQL
IEEE_754_FLAG=-DENABLE_IEEE_754
STD_CFLAGS=-I$(USR_INC) -w 
STD_LD_FLAGS=-L$(USR_LIB) 
HS=algol68g.h config.h diagnostics.h environ.h genie.h gsl.h inline.h mp.h transput.h version.h
CS=acronym.o algol68g.o coercion.o diagnostics.o engine.o generator.o genie.o io.o listing.o math.o moids.o monitor.o mp.o names.o optimised.o options.o parallel.o parser.o plotutils.o prelude.o scanner.o scope.o signal.o sound.o standard.o stowed.o support.o taxes.o transput.o unix.o
FLAGS=$(CPPFLAGS) $(STD_CFLAGS) $(POSIX_THREADS_FLAG) $(PLOTUTILS_FLAG) $(GSL_FLAG) $(CURSES_FLAG) $(IEEE_754_FLAG) $(HTTP_FLAG) $(REGEX_FLAG) $(TERMINFO_FLAG) $(SQL_FLAG) $(CFLAGS)
all:    a68g
a68g:   $(CS)
	@echo "building a68g ..."
	@$(CC) $(CS) $(CFLAGS) $(STD_LD_FLAGS) $(POSIX_THREADS_LIBS) $(PLOTUTILS_LIBS) $(GSL_LIBS) $(CURSES_LIBS) $(SQL_LIBS) -lm $(LD_FLAGS) -o a68g
$(CS): $(HS)
.c.o:
	@echo "compiling $<"
	@$(CC) -c $(FLAGS) $<
clean:
	@rm -f $(CS)
depend:
	@touch $(HS)
