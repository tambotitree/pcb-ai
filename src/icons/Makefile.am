EXTRA_DIST= hand.dat hcurs.dat lcurs.dat lock.dat


# Bundled gschem artwork; no desktop theme or network fetch is needed.
bundled_icon_files = \
	bundled/16x16/actions/attribute-attach.png \
	bundled/16x16/actions/attribute-detach.png \
	bundled/16x16/actions/attribute-show-both.png \
	bundled/16x16/actions/attribute-show-name.png \
	bundled/16x16/actions/attribute-show-value.png \
	bundled/16x16/actions/clone.png \
	bundled/16x16/actions/deselect.png \
	bundled/16x16/actions/gtk-about.png \
	bundled/16x16/actions/gtk-cancel.png \
	bundled/16x16/actions/gtk-close.png \
	bundled/16x16/actions/gtk-copy.png \
	bundled/16x16/actions/gtk-cut.png \
	bundled/16x16/actions/gtk-delete.png \
	bundled/16x16/actions/gtk-edit.png \
	bundled/16x16/actions/gtk-enable-accels.png \
	bundled/16x16/actions/gtk-execute.png \
	bundled/16x16/actions/gtk-find.png \
	bundled/16x16/actions/gtk-go-back.png \
	bundled/16x16/actions/gtk-go-down.png \
	bundled/16x16/actions/gtk-go-forward.png \
	bundled/16x16/actions/gtk-go-up.png \
	bundled/16x16/actions/gtk-goto-bottom.png \
	bundled/16x16/actions/gtk-new.png \
	bundled/16x16/actions/gtk-ok.png \
	bundled/16x16/actions/gtk-open.png \
	bundled/16x16/actions/gtk-paste.png \
	bundled/16x16/actions/gtk-preferences.png \
	bundled/16x16/actions/gtk-print.png \
	bundled/16x16/actions/gtk-properties.png \
	bundled/16x16/actions/gtk-quit.png \
	bundled/16x16/actions/gtk-redo.png \
	bundled/16x16/actions/gtk-refresh.png \
	bundled/16x16/actions/gtk-revert-to-saved.png \
	bundled/16x16/actions/gtk-save-as.png \
	bundled/16x16/actions/gtk-save.png \
	bundled/16x16/actions/gtk-select-all.png \
	bundled/16x16/actions/gtk-undo.png \
	bundled/16x16/actions/gtk-zoom-fit.png \
	bundled/16x16/actions/gtk-zoom-in.png \
	bundled/16x16/actions/gtk-zoom-out.png \
	bundled/16x16/actions/insert-arc.png \
	bundled/16x16/actions/insert-attribute.png \
	bundled/16x16/actions/insert-box.png \
	bundled/16x16/actions/insert-bus.png \
	bundled/16x16/actions/insert-circle.png \
	bundled/16x16/actions/insert-line.png \
	bundled/16x16/actions/insert-net.png \
	bundled/16x16/actions/insert-path.png \
	bundled/16x16/actions/insert-pin.png \
	bundled/16x16/actions/insert-symbol.png \
	bundled/16x16/actions/insert-text.png \
	bundled/16x16/actions/multi-clone.png \
	bundled/16x16/actions/select.png \
	bundled/16x16/actions/symbol-datasheet.png \
	bundled/24x24/actions/object-flip-horizontal.xpm \
	bundled/24x24/actions/object-rotate-left.xpm \
	bundled/48x48/apps/geda-gschem.png \
	bundled/PCB-README \
	bundled/README \
	bundled/fetch-material-icons.sh \
	bundled/pcb-icons.gresource.xml \
	bundled/scalable/actions/help-browser.svg \
	bundled/scalable/actions/insert-image.svg \
	bundled/scalable/actions/web-browser.svg \
	bundled/scalable/actions/window-new.svg

EXTRA_DIST += $(bundled_icon_files)

# Explicit preparation target until the GTK toolbar resource consumer is ported.
# The normal batch build does not require this tool.
GLIB_COMPILE_RESOURCES = glib-compile-resources
pcb-icons.gresource: $(bundled_icon_files)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/bundled \
	  --target=$@ $(srcdir)/bundled/pcb-icons.gresource.xml

CLEANFILES = pcb-icons.gresource
