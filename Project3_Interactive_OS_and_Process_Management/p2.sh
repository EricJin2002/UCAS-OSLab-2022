#!/bin/bash
make clean
make all

folder="/home/stu/jinyang20/Project2_Simple_Kernel/test/test_project2"  
out=".gdbinit"
prefix="add-symbol-file ~/jinyang20/Project2_Simple_Kernel/build/"
: > $out
for file in ${folder}/*
do  
    temp_file=`basename $file .c`
    echo $prefix$temp_file >> $out
done

make debug


# if not work, try this:
# https://stackoverflow.com/questions/20380204/how-to-load-multiple-symbol-files-in-gdb
# define add-symbol-file-auto
#   # Parse .text address to temp file
#   shell echo set \$text_address=$(readelf -WS $arg0 | grep .text | awk '{ print "0x"$5 }') >/tmp/temp_gdb_text_address.txt
# 
#   # Source .text address
#   source /tmp/temp_gdb_text_address.txt
# 
#   #  Clean tempfile
#   shell rm -f /tmp/temp_gdb_text_address.txt
# 
#   # Load symbol table
#   add-symbol-file $arg0 $text_address
# end
# 
# add-symbol-file-auto ~/jinyang20/Project2_Simple_Kernel/build/add