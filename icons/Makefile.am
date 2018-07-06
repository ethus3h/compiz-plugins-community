iconsdir = $(pluginiconsdir)/scalable/apps
icons_DATA =              \
  plugin-boing.svg        \
  plugin-dclick.svg       \
  plugin-dodge.svg        \
  plugin-flash.svg        \
  plugin-headtracking.svg \
  plugin-maze.svg         \
  plugin-mousegestures.svg\
  plugin-lazypointer.svg  \
  plugin-newton.svg       \
  plugin-peek.svg         \
  plugin-prompt.svg       \
  plugin-roach.svg        \
  plugin-rubik.svg        \
  plugin-showrepaint.svg  \
  plugin-visualevent.svg  \
  plugin-wiimote.svg      \
  plugin-wiitrack.svg     \
  plugin-windowcapture.svg

install-data-hook:
	$(shell which gtk-update-icon-cache &> /dev/null &&	\
	gtk-update-icon-cache -f -t $(DESTDIR)$(pluginiconsdir))
