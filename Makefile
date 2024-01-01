#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.1.1.1 2003/10/29 14:14:42 rdp123 Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = dsmcc

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

#CXX      ?= g++
#CXXFLAGS ?= -g -Wall -Woverloaded-virtual

### The directory environment:

PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(DESTDIR)$(call PKGCFG,libdir)
LOCDIR = $(DESTDIR)$(call PKGCFG,locdir)
#DVBDIR = ../../../../DVB
#VDRDIR = ../../../
#LIBDIR = ../../lib
TMPDIR = $(CACHEDIR)

### Allow user defined options to overwrite defaults:
export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

-include $(VDRDIR)/Make.config

CFLAGS += -fPIC
CXXFLAGS += -fPIC
### The version number of VDR (taken from VDR's "config.h"):
APIVERSION = $(call PKGCFG,apiversion)
#APIVERSION = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):
DSMCC_INC:=$(shell pkg-config --cflags libdsmcc)

INCLUDES += -I$(VDRDIR)/include -I$(DVBDIR)/include -I. $(DSMCC_INC)

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

###
DSMCC_LIB:=$(shell pkg-config --libs libdsmcc)

LIBS += -lz $(DSMCC_LIB)

### The object files (add further files here):

OBJS = dsmcc.o dsmcc-monitor.o dsmcc-decoder.o dsmcc-siinfo.o

### Targets:
all: $(SOFILE)

plug: $(SOFILE)

### Implicit rules:

%.o: %.c
	@echo CC $@
	$(Q)$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Targets:

$(SOFILE): $(OBJS)
	@echo LD $@
	$(Q)$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@ 

install-lib: $(SOFILE)
	install -D $^ $(LIBDIR)/$^.$(APIVERSION)

install: install-lib

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
