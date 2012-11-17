TOP_DIR = .

SUBDIRS = \
		  $(TOP_DIR)/src

all:
	$(Q) for subdir in $(SUBDIRS) ; \
		do \
		make -C $${subdir}; \
		done

run:
	$(Q) for subdir in $(SUBDIRS) ; \
		do \
		make -C $${subdir} $@ ; \
		done

clean:
	$(Q) for subdir in $(SUBDIRS) ; \
		do \
		make -C $${subdir} $@ ; \
		done
