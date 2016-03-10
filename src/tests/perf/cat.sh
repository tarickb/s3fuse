#!/bin/sh

rootdir=$1 
count=$2
n=0

while [ "${n}" -lt "${count}" ]
do
    cat ${rootdir}/copy_${n} >> /dev/null
    n=$(expr ${n} + 1)
done

#ls ${rootdir}/
