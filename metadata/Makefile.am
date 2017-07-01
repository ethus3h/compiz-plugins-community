xmldir       = $(metadatadir)

xml_in_files =               \
	boing.xml.in         \
	maze.xml.in          \
	newton.xml.in        \
	roach.xml.in         \
	rubik.xml.in

xml_files    = $(xml_in_files:.xml.in=.xml)
xml_DATA     = $(xml_files)

@INTLTOOL_XML_RULE@

EXTRA_DIST =		\
	$(xml_in_files)

DISTCLEANFILES =	\
	$(xml_files)
	
