
obj-y				:= fabs.o fneg.o types.o udivmodti4.o

obj-$(CONFIG_MATH_EMULATION)	+= math.o fmr.o lfd.o stfd.o \
					fadd.o fadds.o fcmpo.o fcmpu.o \
					fctiw.o fctiwz.o fdiv.o fdivs.o \
					fmadd.o fmadds.o fmsub.o fmsubs.o \
					fmul.o fmuls.o fnabs.o \
					fnmadd.o fnmadds.o fnmsub.o fnmsubs.o \
					fres.o frsp.o frsqrte.o fsel.o lfs.o \
					fsqrt.o	fsqrts.o fsub.o fsubs.o \
					mcrfs.o mffs.o mtfsb0.o mtfsb1.o \
					mtfsf.o mtfsfi.o stfiwx.o stfs.o

obj-$(CONFIG_SPE)		+= efsabs.o efsadd.o efscfd.o efscmpeq.o \
					efscmpgt.o efscmplt.o efsctsf.o efsctsi.o \
					efsctsiz.o efsctuf.o efsctui.o efsctuiz.o \
					efsdiv.o efsmul.o efsnabs.o efsneg.o efssub.o \
					evfsabs.o evfsadd.o evfscmpeq.o evfscmpgt.o \
					evfscmplt.o evfsctsf.o evfsctsi.o evfsctsiz.o \
					evfsctuf.o evfsctui.o evfsctuiz.o evfsdiv.o \
					evfsmul.o evfsnabs.o evfsneg.o evfssub.o \
					efdadd.o efdcfs.o efdcmpeq.o efdcmpgt.o efdcmplt.o \
					efdctsf.o efdctsi.o efdctsidz.o efdctsiz.o efdctuf.o \
					efdctui.o efdctuidz.o efdctuiz.o efddiv.o efdmul.o \
					efdnabs.o efdsub.o

CFLAGS_fabs.o = -fno-builtin-fabs
CFLAGS_math.o = -fno-builtin-fabs
