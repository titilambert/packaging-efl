/*
 * Defines the windows UUID IID_IPersistFile used for links in
 * evil. This is here since uuid.lib is a static only library and
 * libtool does not allow you to link a DLL against a static library.
 */

# define INITGUID
# include <basetyps.h>
DEFINE_OLEGUID(IID_IPersistFile, 0x0000010BL, 0, 0);
