
# require at least autoconf 2.61
AC_PREREQ(2.61)

# Process this file with autoconf to produce a configure script.
AC_INIT([RcppCorels],[0.0.1])

# Ensure C++ is set up as R expects
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
    AC_MSG_ERROR([Could not determine R_HOME.])
fi
CXX=`"${R_HOME}/bin/R" CMD config CXX`
CXXFLAGS=`"${R_HOME}/bin/R" CMD config CXXFLAGS`
AC_LANG(C++)
AC_REQUIRE_CPP
AC_PROG_CC
AC_PROG_CXX

# Default to no GNU GMP
GMPFLAG=

# If gmp.h is found, define GMP
AC_CHECK_HEADER([gmp.h],
   [GMPFLAG=-DGMP],
   [AC_MSG_WARN([gmp.h not found, so skipping GMP resulting in slower code.])])

# If libgmp is found with __gmpz_init symbol, link to GMP
AC_CHECK_LIB(gmp, __gmpz_init, ,
   [AC_MSG_WARN([gmp library not found, so skipping GMP.])])

# Now substitute these variables in src/Makevars.in to create src/Makevars
AC_SUBST(GMP_CFLAGS, ["${GMPFLAG} ${CFLAGS}"])
AC_SUBST(GMP_LIBS, ${LIBS})
AC_OUTPUT(src/Makevars)
