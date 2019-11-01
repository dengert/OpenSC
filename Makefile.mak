SUBDIRS = etc win32 src

default: all

32:
	@echo Making 32 in top
	CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
	$(MAKE) /f Makefile.mak opensc.msi PLATFORM=x86
	MOVE win32\OpenSC.msi OpenSC_win32.msi

64:
	@echo Making 64 in top
	CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86_amd64
	$(MAKE) /f Makefile.mak opensc.msi PLATFORM=x64
	MOVE win32\OpenSC.msi OpenSC_win64.msi

vs32:
	@echo Making VS32 in top
	$(MAKE) /f Makefile.mak all PLATFORM=x86
	@cmd /c "cd win32 && $(MAKE) /nologo /f Makefile.mak opensc.msi PLATFORM=x86"
	MOVE win32\OpenSC.msi OpenSC_win32.msi

vs64:
	@echo Making VS64 in top
	$(MAKE) /f Makefile.mak all
	@cmd /c "cd win32 && $(MAKE) /P /D /nologo /f Makefile.mak opensc.msi"
	MOVE win32\OpenSC.msi OpenSC_win64.msi

opensc.msi:
	@echo Making opensc.msi in top
	# $(MAKE) /f Makefile.mak $@
	@cmd /c "cd win32 && $(MAKE) /nologo /f Makefile.mak opensc.msi"

all clean::
	@echo Making all, clean in top
	@for %i in ( $(SUBDIRS) ) do @cmd /c "cd %i && $(MAKE) /nologo /f Makefile.mak $@"
