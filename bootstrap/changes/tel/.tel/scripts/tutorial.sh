#!/usr/bin/env bash
clear
cowsay 'What tutorial would you like to start?' ; 


#read menu_sel
#tree ~/.tel/tutorials
#clear ; cowsay "you chose $menu_sel"

#files=(~/.tel/tutorials/*)
cd ~/.tel/tutorials
files=(~/.tel/tutorials/*)
filesnames=(*)
for i in $(seq 0 $((${#files[@]}-1))); do
    echo "$i) ${filesnames[$i]}"
done

read -e -p "Select a file by index from the list: " choice

# TODO validate and default choice

#echo "File choice ${files[$choice]}"
clear ; cowsay "you chose  ${files[$choice]}"
clear; ${files[$choice]}


case "$1" in
	-e)
            echo exiting...
            exit 0
        ;;
        # Display the help message
        -h)
            echo "tel-tutorial usage:
	    tel-tutorial 	= start interactive picker
	    "
            exit 0
        ;;
esac
exit 0
