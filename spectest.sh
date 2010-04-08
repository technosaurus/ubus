#!/bin/bash

echo '1 ( foo bar ) n ( ab v) 3 lol g h'
str=$'\v1\v\e(\vfoo\vbar\v\e)\vn\v\e(\vab\vv\v\e)\v3\vlol\vg\vh'
echo $str

nest=0;
key='a'
iskey=1;
IFS=$'\v'
for field in ${str##$'\v'}
do
    if [ "$field" = $'\e)' ]
    then
        nest=$(expr $nest - 1)
        l=$(echo $key| rev| cut -d '_' -f 1 | rev)
        key=${key%'_'$l}
        continue
    fi
    if [ "$field" = $'\e(' ]
    then
        nest=$(expr $nest + 1)
        iskey=1 
        continue
    fi
    if [ $iskey = 1 ]
    then
        lastkey=$field
        key=$key'_'$field
        iskey=0
    else
        iskey=1 
        echo $key"="$field
        l=$(echo $key| rev| cut -d '_' -f 1 | rev)
        key=${key%'_'$l}
    fi
done

