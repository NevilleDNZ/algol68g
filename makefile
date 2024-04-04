# makefile generated on Mon May  5 08:56:18 EST 2008 by ndempsey.
SHELL=/bin/sh
PACKAGE_NAME=algol68g-mk12
USR_BIN=/home/ndempsey/prj/a68/algol68g-mk12
USR_SRC=./source
USR_CHK=./regression-mk12
man_dir=/home/ndempsey/prj/a68/algol68g-mk12
all a68g::
	@cd $(USR_SRC); $(MAKE) CPP_FLAGS="$(CPP_FLAGS)" CFLAGS="$(CFLAGS)" LD_FLAGS="$(LD_FLAGS)"; mv a68g ..; cd ..
install:	all
	@install -m 755 -s a68g $(USR_BIN)
	@install -m 644 doc/man1/a68g.1 $(man_dir)
uninstall:
	@rm -f $(USR_BIN)/a68g $(man_dir)/a68g.1
depend:
	@cd $(USR_SRC); $(MAKE) depend; cd ..
regression:  all
	@cd $(USR_CHK); ./regression $(DIR); cd ..
regression-ref:  all
	@cd $(USR_CHK); ./regression-ref $(DIR); cd ..
clean:
	@cd $(USR_SRC); $(MAKE) clean; cd ..
	@for f in `echo core`; do rm -f $$f; done
distclean mostlyclean maintainer-clean: clean
	@rm -f makefile $(USR_SRC)/makefile
	@rm -f a68g .a68g.log .a68g.tmp .a68g.x .a68g.x.l *.tmp
	./configure -anonymous
	./configure -docs
dist:   distclean
	./configure -dist
TAGS info dvi:
	@echo "target not implemented"
