# makefile generated on Wed Apr 11 20:55:02 NZST 2007 by nevilled.
SHELL=/bin/sh
PACKAGE_NAME=algol68g-mk10
USR_BIN=/home/nevilled/bin
USR_SRC=./source
USR_CHK=./regression-mk10
man_dir=/home/nevilled/prj/a68/algol68g-mk10
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
clean:
	@cd $(USR_SRC); $(MAKE) clean; cd ..
	@for f in `echo core`; do rm -f $$f; done
distclean mostlyclean maintainer-clean: clean
	@rm -f makefile $(USR_SRC)/makefile
	@rm -f a68g .a68g.log .a68g.tmp .a68g.x .a68g.x.l
	./configure -anonymus
dist:   distclean
	@cd ..;tar -czf $(PACKAGE_NAME).tar.gz $(PACKAGE_NAME)
TAGS info dvi:
	@echo "target not implemented"