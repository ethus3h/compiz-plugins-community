xmldir       = $(metadatadir)

if DODGE_PLUGIN
#depends on mousepoll
visualeventxml = visualevent.xml.in
endif

if HEADTRACKING_PLUGIN
#depends on opencv
headtrackingxml = headtracking.xml.in
endif

if VISUALEVENT_PLUGIN
#depends on pango
#depends on xtst
visualeventxml = visualevent.xml.in
endif

if WIIMOTE_PLUGIN
#depends on cwiid
wiimotexml = wiimote.xml.in
endif

if WIITRACK_PLUGIN
#depends on cwiid
wiitrackxml = wiitrack.xml.in
endif

xml_in_files =           \
	boing.xml.in            \
	dclick.xml.in           \	
	flash.xml.in            \
	lazypointer.xml.in      \
	maze.xml.in             \
	mousegestures.xml.in    \
	newton.xml.in           \
	peek.xml.in             \
	prompt.xml.in           \
	roach.xml.in            \
	rubik.xml.in            \
	showrepaint.xml.in      \
	windowcapture.xml.in    \
	$(dodge.xml.in)         \
	$(headtracking.xml.in)  \
	$(visualevent.xml.in)   \
	$(wiimote.xml.in)       \
	$(wiitrack.xml.in)


xml_files    = $(xml_in_files:.xml.in=.xml)
xml_DATA     = $(xml_files)

@INTLTOOL_XML_RULE@

EXTRA_DIST =		\
	$(xml_in_files)

DISTCLEANFILES =	\
	$(xml_files)
