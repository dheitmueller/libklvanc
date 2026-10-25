AUTOMAKE_OPTIONS = foreign
SUBDIRS = src tools tests
EXTRA_DIST = doxygen/libklvanc.doxyconf doxygen/include

docs:
	cd doxygen && doxygen libklvanc.doxyconf

test:
	cd tools && make test
