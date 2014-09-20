dnl The option stuff below is based on the similar code from Automake

# _GNOME_POCKET_MANGLE_OPTION(NAME)
# -------------------------
# Convert NAME to a valid m4 identifier, by replacing invalid characters
# with underscores, and prepend the _GNOME_POCKET_OPTION_ suffix to it.
AC_DEFUN([_GNOME_POCKET_MANGLE_OPTION],
[[_GNOME_POCKET_OPTION_]m4_bpatsubst($1, [[^a-zA-Z0-9_]], [_])])

# _GNOME_POCKET_SET_OPTION(NAME)
# ----------------------
# Set option NAME.  If NAME begins with a digit, treat it as a requested
# Guile version number, and define _GNOME_POCKET_GUILE_VERSION to that number.
# Otherwise, define the option using _GNOME_POCKET_MANGLE_OPTION.
AC_DEFUN([_GNOME_POCKET_SET_OPTION],
[m4_define(_GNOME_POCKET_MANGLE_OPTION([$1]), 1)])

# _GNOME_POCKET_SET_OPTIONS(OPTIONS)
# ----------------------------------
# OPTIONS is a space-separated list of gnome_pocket options.
AC_DEFUN([_GNOME_POCKET_SET_OPTIONS],
[m4_foreach_w([_GNOME_POCKET_Option], [$1], [_GNOME_POCKET_SET_OPTION(_GNOME_POCKET_Option)])])

# _GNOME_POCKET_IF_OPTION_SET(NAME,IF-SET,IF-NOT-SET)
# -------------------------------------------
# Check if option NAME is set.
AC_DEFUN([_GNOME_POCKET_IF_OPTION_SET],
[m4_ifset(_GNOME_POCKET_MANGLE_OPTION([$1]),[$2],[$3])])

dnl GNOME_POCKET_INIT([OPTIONS], [DIR])
dnl ----------------------------
dnl OPTIONS      A whitespace-seperated list of options.
dnl DIR          gnome-pocket submodule directory (defaults to 'gnome-pocket')
AC_DEFUN([GNOME_POCKET_INIT], [
    _GNOME_POCKET_SET_OPTIONS([$1])
    AC_SUBST([GNOME_POCKET_MODULE_DIR],[m4_if([$2],,[gnome-pocket],[$2])])

    AC_REQUIRE([LT_INIT])
    AC_REQUIRE([AC_CHECK_LIBM])
    AC_SUBST(LIBM)
    GNOME_POCKET_MODULES="rest-0.7 goa-1.0 json-glib-1.0 gom-1.0"
    GNOME_POCKET_GIR_INCLUDES="Rest-0.7 Goa-1.0 Json-1.0 Gom-1.0"

    AM_CONDITIONAL([GNOME_POCKET_STATIC],[_GNOME_POCKET_IF_OPTION_SET([static],[true],[false])])

    # gir: gobject introspection support
    AM_CONDITIONAL([GNOME_POCKET_GIR],[ _GNOME_POCKET_IF_OPTION_SET([gir],[true],[false])])
    _GNOME_POCKET_IF_OPTION_SET([gir],[
        GOBJECT_INTROSPECTION_REQUIRE([0.9.6])
    ])

    PKG_CHECK_MODULES(GNOME_POCKET, [ $GNOME_POCKET_MODULES ])
    AC_SUBST(GNOME_POCKET_GIR_INCLUDES)
    AC_SUBST(GNOME_POCKET_SOURCES)
])
