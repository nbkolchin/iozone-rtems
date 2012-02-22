#
#  Makefile.leaf,v 1.7 2002/07/22 22:56:09 joel Exp
#
# Templates/Makefile.leaf
# 	Template leaf node Makefile
#

# C source names, if any, go here -- minus the .c
C_PIECES=init iozone
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# C++ source names, if any, go here -- minus the .cc
#CC_PIECES=exc
#CC_FILES=$(CC_PIECES:%=%.cc)
#CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=system.h

# Assembly source names, if any, go here -- minus the .S
#S_PIECES=
#S_FILES=$(S_PIECES:%=%.S)
#S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

PGMS=${ARCH}/nil.exe

#  List of RTEMS Classic API Managers to be included in the application
#  goes here. Use:
#     MANAGERS=all
# to include all RTEMS Classic API Managers in the application or
# something like this to include a specific set of managers.
#     MANAGERS=io event message rate_monotonic semaphore timer
MANAGERS=all

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

#
# (OPTIONAL) Add local stuff here using +=
#

DEFINES  += -DNAME='"rtems"' -DHAVE_ANSIC_C -DNO_MADVISE \
	    -DDONT_HAVE_O_DIRECT
CPPFLAGS += -I../../ppc-altera
CFLAGS   += 

#
# CFLAGS_DEBUG_V are used when the `make debug' target is built.
# To link your application with the non-optimized RTEMS routines,
# uncomment the following line:
# CFLAGS_DEBUG_V += -qrtems_debug
#

# LD_PATHS  += xxx-your-EXTRA-library-paths-go-here, if any
# LD_LIBS   += xxx-your-libraries-go-here eg: -lvx
# LD_PATHS += -L.
LD_LIBS += -ltelnetd
    

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

# CLEAN_ADDITIONS += xxx-your-debris-goes-here
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(PGMS)

# libredef.a: redef.o
#	$(AR) rc $@ $^
#${ARCH}/nil.exe: libredef.a

${ARCH}/nil.exe: ${OBJS} ${LINK_FILES} ${LD_PRE_LIBS}
	echo $(CXX)
	$(make-cxx-exe)

# Install the program(s), appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
install:  all
	$(INSTALL_VARIANT) -m 555 ${PGMS} ${PROJECT_RELEASE}/bin
