#!/bin/sh
# various patches for making a final dist tar ball
echo "Executing pre-dist script..."
version=`date "+%Y%m%d"`
sed "s/VERSION_STRING/${version}/g" > /tmp/version_patch.diff << 'EOF'
--- config.h.in	Wed Apr 28 10:08:27 2004
+++ a	Wed Apr 28 10:08:59 2004
@@ -1,7 +1,7 @@
 /* config.h.in.  Generated from configure.in by autoheader.  */
 
 /* package's version */
-#define CMDLINE_PARSER_VERSION "devel"
+#define CMDLINE_PARSER_VERSION "VERSION_STRING"
 
 /* Define to 1 if you have iconv call. */
 #define HAVE_ICONV 1
EOF
patch config.h.in < /tmp/version_patch.diff
rm /tmp/version_patch.diff

. script/fileperm.sh
