 #!/bin/bash

wget "https://en.wikipedia.org/wiki/Lists_of_musicians"

for line in $(grep 'List of' Lists_of_musicians)
do 
    ref=$(echo $line | grep -P '(?<=href=\")([^\"]*)')
    ref=${ref:6:-1}
    ref=${ref/#\//}
    [[ ! -z $ref ]] && wget https://en.wikipedia.org/$ref 
done

rm $(ls | grep -v -e List -e crawl -e \.add -e \.txt)
rm index*
rm Lists_of_musicians*
rm *video_game*

for file in $(ls | grep List)
do 
    grep -Pzo '(?s)firstHeading.*See_also' $file > $file.splice
    [[ ! -s $file.splice ]] && grep -Pzo '(?s)firstHeading.*id="References"' $file > $file.splice
    [[ ! -s $file.splice ]] && grep -Pzo '(?s)firstHeading.*' $file > $file.splice
done

rm $(ls | grep -v -e splice -e crawl -e \.add -e \.txt)

grep -P '<(li|td).{0,100}title=' *splice | grep -v http | grep -oP '(?<=>)[^<]+' | grep -Pv '[\d*]' | grep -P '^[\w ]{4,50}' | sort | uniq > music.txt

rm $(ls | grep -v -e crawl -e \.txt -e \.add)

cat music.add >> music.txt
