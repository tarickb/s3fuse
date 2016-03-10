#!/bin/sh

rootdir=$1 
count=$2
n=0

while [ "${n}" -lt "${count}" ]
do
    mv ${rootdir}/create_${n}  ${rootdir}/mv_${n} 
    n=$(expr ${n} + 1)
done

#ls ${rootdir}/
