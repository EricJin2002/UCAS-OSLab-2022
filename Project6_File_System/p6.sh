#!/bin/bash
make clean
make all

folder="./build/"  
out=".gdbinit"
prefix="add-symbol-file $(pwd)/build/"
: > $out
echo $prefix"main 0x50202000" >> $out
for file in ${folder}/*.txt
do  
    temp_file=`basename $file .txt`
    if [ $temp_file == "test" ]; then
        echo $prefix$temp_file >> $out
    fi
done

# make debug
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