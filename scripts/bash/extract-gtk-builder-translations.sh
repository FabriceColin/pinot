#!/bin/bash
# Extracts single line only translatable strings from the XML file

YEAR=`date +%Y`

echo "#ifndef _BUILDERTRANSLATIONS_HH"
echo "#define _BUILDERTRANSLATIONS_HH"
echo
echo "#include <string>"
echo
echo "static struct std::string g_builderTranslations[] = {"
grep -w property UI/GTK3/metase-gtk3.gtkbuilder | grep 'translatable="yes"' | grep "\/property" | sed -e "s/.*\">\(.*\)<\/property>/_(\"\1\"),/g"
echo "\"\""
echo "};"
echo
echo "#endif // _BUILDERTRANSLATIONS_HH"
