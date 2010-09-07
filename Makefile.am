SUBDIRS = src inc

pcfiles = ixion-@IXION_API_VERSION@.pc
ixion-@IXION_API_VERSION@.pc: ixion.pc
		@cp -f $< $@

pkgconfig_DATA = $(pcfiles)
pkgconfigdir = $(libdir)/pkgconfig

