#!/bin/sh

ROOTDIR=$1/perf
COUNT=100

echo "prepare: removing all ..."
rm -rf ${ROOTDIR}
mkdir -p ${ROOTDIR}

echo "--- create 1M file"
time dd if=/dev/zero of=${ROOTDIR}/1m bs=$(expr 1 \* 1024 \* 1024) count=1
echo "--- done"

echo "--- create 5M file"
time dd if=/dev/zero of=${ROOTDIR}/5m bs=$(expr 5 \* 1024 \* 1024) count=1
echo "--- done"

echo "--- create 10M file"
time dd if=/dev/zero of=${ROOTDIR}/10m bs=$(expr 10 \* 1024 \* 1024) count=1
echo "--- done"

echo "--- create 50M file"
time dd if=/dev/zero of=${ROOTDIR}/50m bs=$(expr 50 \* 1024 \* 1024) count=1
echo "--- done"

echo "--- create 100M file"
time dd if=/dev/zero of=${ROOTDIR}/100m bs=$(expr 100 \* 1024 \* 1024) count=1
echo "--- done"

echo "--- create 200M file"
time dd if=/dev/zero of=${ROOTDIR}/200m bs=$(expr 200 \* 1024 \* 1024) count=1
echo "--- done"

echo "--- cat 1M file"
time cat ${ROOTDIR}/1m > /dev/null
echo "--- done"

echo "--- cat 5M file"
time cat ${ROOTDIR}/5m > /dev/null
echo "--- done"

echo "--- cat 10M file"
time cat ${ROOTDIR}/10m > /dev/null
echo "--- done"

echo "--- cat 50M file"
time cat ${ROOTDIR}/50m > /dev/null
echo "--- done"

echo "--- cat 100M file"
time cat ${ROOTDIR}/100m > /dev/null
echo "--- done"

echo "--- cat 200M file"
time cat ${ROOTDIR}/200m > /dev/null
echo "--- done"

echo "prepare: removing all ..."
rm -rf ${ROOTDIR}
mkdir -p ${ROOTDIR}

echo "--- create new non-zero byte ${COUNT} files"
time sh create.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "--- copy non-zero byte ${COUNT} files"
time sh copy.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "--- cat non-zero byte ${COUNT} files"
time sh cat.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "--- mv non-zero byte ${COUNT} files"
time sh mv.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "--- rm non-zero byte ${COUNT} files"
time sh rm.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "prepare: removing all ..."
rm -rf ${ROOTDIR}
mkdir -p ${ROOTDIR}

echo "--- touch zero byte ${COUNT} files"
time sh touch.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "--- touch already exist ${COUNT} files"
time sh touch.sh ${ROOTDIR} ${COUNT}
echo "--- done"

echo "prepare: create ${COUNT} files ..."
rm -rf ${ROOTDIR}
mkdir -p ${ROOTDIR}
sh create.sh ${ROOTDIR} ${COUNT}

echo "--- mv directory"
time sh mvdir.sh ${ROOTDIR} 
echo "--- done"

echo "--- rm directory and ${COUNT} files"
time sh rmdir.sh ${ROOTDIR} 
echo "--- done"
