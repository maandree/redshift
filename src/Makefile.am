
SUBDIRS = redshift-gtk

# I18n
localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	redshift.c redshift.h \
	settings.c settings.h \
	colorramp.c colorramp.h \
	config-ini.c config-ini.h \
	location-manual.c location-manual.h \
	solar.c solar.h \
	systemtime.c systemtime.h \
	adjustments.h \
	gamma-common.c gamma-common.h \
	opt-parser.c opt-parser.h \
	hooks.c hooks.h \
	gamma-dummy.c gamma-dummy.h

EXTRA_redshift_SOURCES = \
	gamma-drm.c gamma-drm.h \
	gamma-randr.c gamma-randr.h \
	gamma-vidmode.c gamma-vidmode.h \
	gamma-quartz.c gamma-quartz.h \
	fake-quartz.c fake-quartz.h \
	gamma-w32gdi.c gamma-w32gdi.h \
	fake-w32gdi.c fake-w32gdi.h \
	location-geoclue.c location-geoclue.h

AM_CFLAGS =
redshift_LDADD = @LIBINTL@
redshift_LDFLAGS =
EXTRA_DIST =

if ENABLE_DRM
redshift_SOURCES += gamma-drm.c gamma-drm.h
AM_CFLAGS += $(DRM_CFLAGS)
redshift_LDADD += \
	$(DRM_LIBS) $(DRM_CFLAGS)
endif

if ENABLE_RANDR
redshift_SOURCES += gamma-randr.c gamma-randr.h
AM_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
redshift_LDADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)
endif

if ENABLE_VIDMODE
redshift_SOURCES += gamma-vidmode.c gamma-vidmode.h
AM_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
redshift_LDADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)
endif

if ENABLE_QUARTZ
redshift_SOURCES += gamma-quartz.c gamma-quartz.h
if FAKE_QUARTZ
redshift_SOURCES += fake-quartz.c fake-quartz.h
else
redshift_LDFLAGS += -I/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/CoreGraphics.framework/Versions/A/Headers \
                    -F/System/Library/Frameworks/ApplicationServices.framework \
                    -framework ApplicationServices
endif
endif

if ENABLE_WINGDI
redshift_SOURCES += gamma-w32gdi.c gamma-w32gdi.h
if FAKE_W32GDI
redshift_SOURCES += fake-w32gdi.c fake-w32gdi.h
else
redshift_LDADD += -lgdi32
endif
endif


if ENABLE_GEOCLUE
redshift_SOURCES += location-geoclue.c location-geoclue.h
AM_CFLAGS += \
	$(GEOCLUE_CFLAGS) $(GEOCLUE_LIBS) \
	$(GLIB_CFLAGS) $(GLIB_LIBS)
redshift_LDADD += \
	$(GEOCLUE_LIBS) $(GEOCLUE_CFLAGS)
	$(GLIB_LIBS) $(GLIB_CFLAGS)
endif
