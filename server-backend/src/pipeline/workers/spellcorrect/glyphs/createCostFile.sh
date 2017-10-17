#!/bin/bash

letters=("" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" "a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k" "l" "m" "n" "o" "p" "q" "r" "s" "t" "u" "v" "w" "x" "y" "z" "A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "Y" "Z" "ä" "ö" "ü" "ß" "Ä" "Ö" "Ü" "!" "\" """ "\%" "\\\&" "/" "(" ")" "=" "\?" "{" "[" "]" "}" "\\\\" "-" "_" "+" "\*" "," ";" "\." ":" "\@" " ")

for l1 in "${letters[@]}"
do 
    for l2 in "${letters[@]}"
    do 
        #for l3 in "${letters[@]}"
        #do 
            #for l4 in "${letters[@]}"
            #do
                text="$l1$l2$l3$l4"
                echo "$text"
                sed -i.bak "s/Text/${text}/" glyphs.svg
                inkscape --export-png="images/${text}.png" --export-area-drawing -f glyphs.svg -z
                mv glyphs.svg.bak glyphs.svg         
            #done
        #done
    done
done

pushd images
./compareImages.sh
popd
