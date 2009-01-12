servicedir = $(DBUS_SERVICES_DIR)
service_in_files = gmediadb.service.in
service_DATA = gmediadb.service

CLEANFILES = $(service_DATA)
EXTRA_DIST = $(service_in_files)

%.service: %.service.in
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@
