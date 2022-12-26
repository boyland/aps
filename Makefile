SUBDIRS= utilities parse analyze aps2scala apscpp
DOCKERTAG= boylanduwm/aps

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

dockerbuild: install
	sudo docker build . -t ${DOCKERTAG}

dockerpush:
	docker push ${DOCKERTAG}

distclean: realclean
	rm -rf bin lib

.PHONY: install clean realclean distclean

