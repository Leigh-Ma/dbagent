#!/bin/bash

#######################################################################################
#generate table_fields.c, table_init.cc
#######################################################################################
out='table_info.c'
cat << EOF > $out
/*
*Warning!!!!!
*This file is generated by script file ---- expand_tables.sh
*Do not modify!
*/

#include "table_pub.h"

EOF


cat tables.h | awk 'BEGIN { FS = "[,\(\)]"; table_no = 0 } { print $1, $2, $3, $4, $5 }' | awk '
{
if($1 == "TABLE_BEGIN") {
print "/* Field descriptions of table @"$2" */"
print "TF tf_"$2"[] = {"
}
if($1 == "TABLE_END") {
print "};/* End @"$2" */"
print "const int tfn_"$2" = sizeof(tf_"$2")/sizeof(TF);"
printf "TI ti_"$2" = {TNO_"toupper($2)",\t sizeof("$2"),\t tf_"$2",\t TFN_"toupper($2)",\t \""tolower($2)"s\",\t \""$2"\",\t \""tolower($2)"_id\"};\n\n\n"
}
if($1 == "FIELD") {
printf "\t{\"%-15s ,\t       sizeof(%-7s),\t %5d,\t sizeof(%-7s),\t FT_%-7s, },\n", $3"\"",     $2,  1, $2, $2
}
if($1 == "FIELD_A") {
printf "\t{\"%-15s ,\t %3d * sizeof(%-7s),\t %5d,\t sizeof(%-7s),\t FT_%-7s, },\n", $3"\"", $4, $2, $4, $2, $2
}
}' >> $out


#######################################################################################
#generate table_info.h, table_fields_init.inc, table_info.inc
#######################################################################################
out='table_info.h'
cat << EOF > $out
#ifndef _TABLE_INFO_H
#define _TABLE_INFO_H
/*
*Warning!!!!!
*This file is generated by script file ---- expand_tables.sh
*Do not modify!
*/

EOF


cat tables.h | awk 'BEGIN { FS = "[,\(\)]" } { print $1, $2, $3, $4, $5 }' | awk '
BEGIN{ table_no = 0; printf "\t/*Automatic generated file. Do not modify! */\n" > "table_info_init.inc"}
{
if($1 == "TABLE_BEGIN") {
table_no = 1 + table_no
print "/* Field descriptions of table @"$2" \t\t*/"
print "#define TNO_"toupper($2)"\t"table_no
print "#define TNO_"$2"\t"table_no "\t\t/* alternative \t*/"
print "#define TFN_"toupper($2)"\t\t ( sizeof(tf_"$2")/sizeof(TF) )"
print "extern  TF tf_"$2"[];"
print "extern  const int tfn_"$2";"
print "extern  TI ti_"$2";\n\n"
line = "\t table_info_init(&ti_"$2", tf_"$2", tfn_"$2");\n"
printf "%s", line > "table_info_init.inc"
}
}
END{
print "#define TNO_MINIMUM\t 1 \n"
print "#define TNO_MAXIMUM\t "table_no"\n"
}' >> $out

echo "#endif" >> $out