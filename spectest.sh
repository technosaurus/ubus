#!/bin/bash
#sorry about the bashim. too lazy. its just a test anyway

function get_tail(){
    echo $1| rev| cut -d '_' -f 1 | rev
}
function drop_tail(){
    l='_'$(get_tail $1)
    echo ${1%$l}
}
function parse(){
    astack='';
    key='a'
    iskey=1;
    oldifs=$IFS
    IFS=$'\v'
    for field in $1
    do
        if [ "$field" = $'\e{' ]
        then
            astack=$astack'_s';
            iskey=1 
            continue
        fi
        if [ "$field" = $'\e}' ]
        then
            astack=$(drop_tail $astack);
            key=$(drop_tail $key);
            continue
        fi
        if [ "$field" = $'\e[' ]
        then
            astack=$astack'_a';
            key=$key'_0'
            iskey=0;
            continue
        fi
        if [ "$field" = $'\e]' ]
        then
            astack=$(drop_tail $astack);
            key=$(drop_tail $key);
            key=$(drop_tail $key);
            if [ "$(get_tail $astack)" = 's' ]
            then
                iskey=1;
            fi
            continue
        fi
        if [ $iskey = 1 ] 
        then
            key=$key'_'$field
            iskey=0
        else
            echo $key"="$field
            if [ $(get_tail $astack) = 's' ]
            then
                iskey=1 
                l=$(echo $key| rev| cut -d '_' -f 1 | rev)
                key=${key%'_'$l}
            fi
            if [ $(get_tail $astack) = 'a' ]
            then
                l=$(echo $key| rev| cut -d '_' -f 1 | rev)
                key=${key%'_'$l}'_'$(expr $l + 1)
            fi
        fi
    done
    IFS=$oldifs
}


echo
echo '==='
echo
echo 'json:  [ { name:maier,sex:male,cars:[ porsche,benz ] } ]'
str=$'\e[\v\e{\vname\vmaier\vsex\vmale\vcars\v\e[\vporsche\vbenz\v\e]\v\e}\v\e]\v'
echo 'data: '$str
echo 'parsed:'
parse $str
echo
echo '==='
echo
echo 'json:  [ bla, bla , bla , bla,bla,[ bla,bla]]'
str=$'\e[\vbla\vbla\vbla\vbla\vbla\v\e[\vbla\vbla\v\e]\v\e]\v'
echo 'data:' $str
echo 'parsed:'
parse $str