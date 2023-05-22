TARGETS = sim fw


.PHONY:	all sim fw clean spotless

all:	$(TARGETS)

sim:
	$(MAKE) -f Makefile.sim

fw:
	$(MAKE) -f Makefile.fw

clean:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n clean; done

spotless:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n spotless; done
