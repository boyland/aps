SUBDIRS= parse analyze aps2scala 
install:
	-mkdir -p lib bin
	for d in ${SUBDIRS}; do \
	  (cd $$d; ${MAKE} install); \
	done

clean:
	for d in ${SUBDIRS}; do \
	  (cd $$d; ${MAKE} clean); \
	done

# This cleans after an install: it does a realclean
# which gets rid of generated .h files as well.
realclean:
	for d in ${SUBDIRS}; do \
	  (cd $$d; ${MAKE} realclean); \
	done

distclean: realclean
	rm -rf bin lib

.PHONY: install clean realclean distclean

