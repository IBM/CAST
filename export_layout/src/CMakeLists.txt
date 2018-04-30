install(FILES export_layout.c
	COMPONENT export_layout
	DESTINATION export_layout/modules)

install(FILES export_layout.makefile
	COMPONENT export_layout
	DESTINATION export_layout/modules
	RENAME Makefile)
