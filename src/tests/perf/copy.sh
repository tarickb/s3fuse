#!/bin/sh

rootdir=$1 
count=$2
n=0

while [ "${n}" -lt "${count}" ]
do
    cp ${rootdir}/create_${n} ${rootdir}/copy_${n}
    n=$(expr ${n} + 1)
done

#ls ${rootdir}/
