#!/bin/sh -xe

echo "check dir"
ls $1 || exit 1 

ROOTDIR=$1/test
DATADIR=$1/test/data

if [ ! -d ${ROOTDIR} ]; then
    mkdir ${ROOTDIR} || exit 1
else
    rm -r ${ROOTDIR}/*
fi
if [ ! -d ${DATADIR}/ ]; then
    mkdir ${DATADIR} || exit 1
fi
#install -m 755 issue_4 ${ROOTDIR}/ || exit 1
install -m 755 test.sh ${ROOTDIR}/ || exit 1
install -m 755 perf/* ${ROOTDIR}/ || exit 1

cd ${ROOTDIR}

# s3fuse tests
echo "--- s3fuse unit test ---"
date
#echo "test" > ./issue_4_test || exit 1
#./issue_4 ./issue_4_test || exit 1

# integration tests
echo "--- integration test ---"
echo "--- step 1 ---"

echo "redirect file"
echo "test message" > ${DATADIR}/redirect_test1 || exit 1
for i in `seq 1 30`
do
   echo "test message ${i}" >> ${DATADIR}/redirect_test2  || exit 1
done
if [ ! -f ${DATADIR}/redirect_test1 ] || [ ! -f ${DATADIR}/redirect_test2 ]; then
   exit 1
fi

echo "verify line count in file"
if [ "30" -ne "$(wc -l ${DATADIR}/redirect_test2 | awk '{print $1}')" ]; then
   exit 1
fi

echo "copy file"
cp ${DATADIR}/redirect_test1 ${DATADIR}/redirect_test3
cp ${DATADIR}/redirect_test2 ${DATADIR}/redirect_test4
if [ ! -f ${DATADIR}/redirect_test1 ] || [ ! -f ${DATADIR}/redirect_test2 ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/redirect_test3 ] || [ ! -f ${DATADIR}/redirect_test4 ]; then
   exit 1
fi
 
echo "cat file"
cat ${DATADIR}/redirect_test2 > /dev/null || exit 1
cat ${DATADIR}/redirect_test4 > /dev/null || exit 1

echo "check files"
ls ${DATADIR} || exit 1 

echo "delete files with glab"
rm ${DATADIR}/redirect_test* || exit 1
if [ -f ${DATADIR}/redirect_test1 ] || [ -f ${DATADIR}/redirect_test2 ]; then
   exit 1
fi
if [ -f ${DATADIR}/redirect_test3 ] || [ -f ${DATADIR}/redirect_test4 ]; then
   exit 1
fi

echo "touch file"
touch ${DATADIR}/touch_test || exit 1
if [ ! -f ${DATADIR}/touch_test ]; then 
  exit 1
fi
touch ${DATADIR}/touch_test || exit 1

echo "redirect file"
for i in `seq 1 30`
do
   echo "test message ${i}" >> ${DATADIR}/touch_test  || exit 1
done

echo "rename file"
mv ${DATADIR}/touch_test ${DATADIR}/touch_test2 || exit 1
if [ -f ${DATADIR}/touch_test ] || [ ! -e ${DATADIR}/touch_test2 ] ; then
   exit 1
fi

echo "verify line count in file"
if [ "30" -ne "$(wc -l ${DATADIR}/touch_test2 | awk '{print $1}')" ]; then
   exit 1
fi

echo "delete file"
rm ${DATADIR}/touch_test2 || exit 1
if [ -f ${DATADIR}/touch_test2 ]; then
   exit 1
fi

echo "make directory"
mkdir ${DATADIR}/dir_test || exit 1
if [ ! -d ${TEST_DIR} ]; then
   exit 1
fi

echo "rename directory"
mv ${DATADIR}/dir_test ${DATADIR}/dir_test2 || exit 1
if [ -d ${DATADIR}/dir_test ] || [ ! -d ${DATADIR}/dir_test2 ]; then
   exit 1
fi

echo "redirect_file"
for i in `seq 1 30`
do
   echo "test message ${i}" >> ${DATADIR}/dir_test2/redirect_test  || exit 1
done
if [ ! -f ${DATADIR}/dir_test2/redirect_test ]; then
   exit 1
fi

echo "rename directory"
mv ${DATADIR}/dir_test2 ${DATADIR}/dir_test3 || exit 1
if [ -d ${DATADIR}/dir_test2 ] || [ ! -d ${DATADIR}/dir_test3 ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/dir_test3/redirect_test ]; then
   exit 1
fi

echo "verify line count in file"
if [ "30" -ne "$(wc -l ${DATADIR}/dir_test3/redirect_test | awk '{print $1}')" ]; then
   exit 1
fi

echo "check files"
ls -R ${DATADIR} || exit 1 

echo "remove directory and file"
rm -r ${DATADIR}/dir_test3 || exit 1
if [ -f ${DATADIR}/dir_test3/redirect_test ] || [ -d ${DATADIR}/dir_test3 ]; then
   exit 1
fi

echo "--- step 2 ---"

echo "make directory"
mkdir ${DATADIR}/dir_test || exit 1
if [ ! -d ${DATADIR}/dir_test ]; then
   exit 1
fi

echo "rename directory"
mv ${DATADIR}/dir_test ${DATADIR}/dir_test2 || exit 1
if [ -d ${DATADIR}/dir_test ] || [ ! -d ${DATADIR}/dir_test2 ]; then
   exit 1
fi

echo "remove directory"
rmdir ${DATADIR}/dir_test2 || exit 1
if [ -d ${DATADIR}/dir_test2 ]; then
   exit 1
fi

echo "--- step 3 ---"

echo "redirect file"
echo -n "ABCDEFGHIJ" >  ${DATADIR}/redirect_test || exit 1
if [ ! -f ${DATADIR}/redirect_test ]; then
   exit 1
fi
if [ "ABCDEFGHIJ" != $(cat ${DATADIR}/redirect_test) ]; then
   exit 1
fi
echo "HELLO" >  ${DATADIR}/redirect_test2 || exit 1
echo "1234579" >>  ${DATADIR}/redirect_test2 || exit 1
if [ ! -f ${DATADIR}/redirect_test2 ]; then
   exit 1
fi
if [ "HELLO" != $(sed -n '1,1p' ${DATADIR}/redirect_test2) ]; then
   exit 1
fi
if [ "1234579" != $(sed -n '2,2p' ${DATADIR}/redirect_test2) ]; then
   exit 1
fi

echo "make directory"
mkdir ${DATADIR}/dir_test || exit 1
if [ ! -d ${DATADIR}/dir_test ]; then
   exit 1
fi

echo "copy directory"
cp -r ${DATADIR}/dir_test ${DATADIR}/dir_test2
if [ ! -d ${DATADIR}/dir_test ] || [ ! -d ${DATADIR}/dir_test2 ]; then
   exit 1
fi

echo "check files"
ls -R ${DATADIR} || exit 1 

echo "remove directory"
rmdir ${DATADIR}/dir_test2 || exit 1
if [ -d ${DATADIR}/dir_test2 ]; then
   exit 1
fi

echo "--- step 4 ---"

echo "make directory"
mkdir ${DATADIR}/dir_test/dir_test || exit 1
if [ ! -d ${DATADIR}/dir_test/dir_test ]; then
   exit 1
fi

echo "redirect file"
echo -n "ABCDEFGHIJ" > ${DATADIR}/dir_test/redirect_test || exit 1
echo -n "ABCDEFGHIJ" > ${DATADIR}/dir_test/redirect_test2 || exit 1
echo -n "ABCDEFGHIJ" > ${DATADIR}/dir_test/dir_test/redirect_test || exit 1
echo -n "ABCDEFGHIJ" > ${DATADIR}/dir_test/dir_test/redirect_test2 || exit 1
if [ ! -f ${DATADIR}/dir_test/redirect_test ] || [ ! -f ${DATADIR}/dir_test/redirect_test2 ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/dir_test/dir_test/redirect_test ] || [ ! -f ${DATADIR}/dir_test/dir_test/redirect_test2 ]; then
   exit 1
fi

echo "copy directory and file"
cp -r ${DATADIR}/dir_test ${DATADIR}/dir_test3 || exit 1
if [ ! -d ${DATADIR}/dir_test ] || [ ! -d ${DATADIR}/dir_test3 ]; then
   exit 1
fi
if [ ! -d ${DATADIR}/dir_test/dir_test ] || [ ! -d ${DATADIR}/dir_test3/dir_test ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/dir_test/redirect_test ] || [ ! -f ${DATADIR}/dir_test3/redirect_test ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/dir_test/redirect_test2 ] || [ ! -f ${DATADIR}/dir_test3/redirect_test2 ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/dir_test/dir_test/redirect_test ] || [ ! -f ${DATADIR}/dir_test3/dir_test/redirect_test ]; then
   exit 1
fi
if [ ! -f ${DATADIR}/dir_test/dir_test/redirect_test2 ] || [ ! -f ${DATADIR}/dir_test3/dir_test/redirect_test2 ]; then
   exit 1
fi

echo "rename directory and file"
mv ${DATADIR}/dir_test ${DATADIR}/dir_test4
if [ -d ${DATADIR}/dir_test ] || [ ! -d ${DATADIR}/dir_test4 ]; then
   exit 1
fi
if [ -d ${DATADIR}/dir_test/dir_test ] || [ ! -d ${DATADIR}/dir_test4/dir_test ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test/redirect_test ] || [ ! -f ${DATADIR}/dir_test4/redirect_test ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test/redirect_test2 ] || [ ! -f ${DATADIR}/dir_test4/redirect_test2 ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test/dir_test/redirect_test ] || [ ! -f ${DATADIR}/dir_test4/dir_test/redirect_test ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test/dir_test/redirect_test2 ] || [ ! -f ${DATADIR}/dir_test4/dir_test/redirect_test2 ]; then
   exit 1
fi

echo "cat files"
cat ${DATADIR}/redirect_test > /dev/null || exit 1
cat ${DATADIR}/redirect_test2 > /dev/null || exit 1
cat ${DATADIR}/dir_test4/redirect_test > /dev/null || exit 1
cat ${DATADIR}/dir_test4/redirect_test2 > /dev/null || exit 1
cat ${DATADIR}/dir_test4/dir_test/redirect_test > /dev/null || exit 1
cat ${DATADIR}/dir_test4/dir_test/redirect_test2 > /dev/null || exit 1

echo "check files"
ls -R ${DATADIR} || exit 1 

echo "remove directory and file"
rm ${DATADIR}/redirect_test ${DATADIR}/redirect_test2 || exit 1
rm -r ${DATADIR}/dir_test3 ${DATADIR}/dir_test4 || exit 1
if [ -f ${DATADIR}/redirect_test ] || [ -f ${DATADIR}/redirect_test ]; then
   exit 1
fi
if [ -d ${DATADIR}/dir_test3 ] || [ -d ${DATADIR}/dir_test4 ]; then
   exit 1
fi
if [ -d ${DATADIR}/dir_test3/dir_test ] || [ -d ${DATADIR}/dir_test4/dir_test ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test3/redirect_test ] || [ -f ${DATADIR}/dir_test4/redirect_test ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test3/redirect_test2 ] || [ -f ${DATADIR}/dir_test4/redirect_test2 ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test3/dir_test/redirect_test ] || [ -f ${DATADIR}/dir_test4/dir_test/redirect_test ]; then
   exit 1
fi
if [ -f ${DATADIR}/dir_test3/dir_test/redirect_test2 ] || [ -f ${DATADIR}/dir_test4/dir_test/redirect_test2 ]; then
   exit 1
fi

echo "--- step 5 ---"

echo "redirect file"
echo -n "ABCDEFGHIJ" >  ${DATADIR}/redirect_test || exit 1
if [ ! -f ${DATADIR}/redirect_test ]; then
   exit 1
fi

echo "chmod file"
prev=$(stat --format=%a ${DATADIR}/redirect_test)
chmod 777 ${DATADIR}/redirect_test
if [ $(stat --format=%a ${DATADIR}/redirect_test) == ${prev} ]; then
  exit 1
fi

echo "remove file"
rm ${DATADIR}/redirect_test 
if [ -f ${DATADIR}/redirect_test ]; then
   exit 1
fi

echo "--- step 6 ---"

echo "make directory"
mkdir ${DATADIR}/dir_test || exit 1
if [ ! -d ${DATADIR}/dir_test ]; then
   exit 1
fi

echo "chmod dir"
prev=$(stat --format=%a ${DATADIR}/dir_test)
chmod 700 ${DATADIR}/dir_test
if [ $(stat --format=%a ${DATADIR}/dir_test) == ${prev} ]; then
  exit 1
fi

echo "remove directory"
rmdir ${DATADIR}/dir_test
if [ -f ${DATADIR}/dir_test ]; then
   exit 1
fi

echo "--- step 7 ---"

for i in `seq 1 5`
do
    for j in `seq 1 50`
    do
       echo "test message ${i} ${j}" >> ${DATADIR}/redirect_test  || exit 1
    done
done
if [ ! -f ${DATADIR}/redirect_test ]; then
   exit 1
fi
echo "cat file" 
cat ${DATADIR}/redirect_test > /dev/null || exit 1
echo "remove file"
rm ${DATADIR}/redirect_test || exit 1
if [ -f ${DATADIR}/redirect_test ]; then
   exit 1
fi

echo "--- step 8 ---"

for loop in `seq 1 5`
do
    touch ${DATADIR}/t || exit 1
    if [ ! -f ${DATADIR}/t ]; then
       exit 1
    fi
    mv ${DATADIR}/t ${DATADIR}/u || exit 1
    if [ -f ${DATADIR}/t ] || [ ! -f ${DATADIR}/u ]; then
        exit 1
    fi
    rm ${DATADIR}/u || exit 1
    if [ -f ${DATADIR}/t ]; then
        exit 1
    fi
    mkdir ${DATADIR}/t || exit 1
    if [ ! -d ${DATADIR}/t ]; then
        exit 1
    fi
    mv ${DATADIR}/t ${DATADIR}/u || exit 1
    if [ -d ${DATADIR}/t ] || [ ! -d ${DATADIR}/u ]; then
        exit 1
    fi
    rm -r ${DATADIR}/u || exit 1
    if [ -d ${DATADIR}/u ]; then
        exit 1
    fi
done

echo "--- step 9 ---"

echo "symbolic link"
mkdir ${DATADIR}/test_dir
echo "hoge" > ${DATADIR}/test_dir/src1 || exit 1
echo "hoge" > ${DATADIR}/test_dir/src2 || exit 1
if [ ! -f ${DATADIR}/test_dir/src1 ] || [ ! -f ${DATADIR}/test_dir/src2 ]; then 
    exit 1
fi
ln -s ${DATADIR}/test_dir/src1 ${DATADIR}/test_dir/dst1 || exit 1
if [ ! -L ${DATADIR}/test_dir/dst1 ]; then
    exit 1
fi
cat ${DATADIR}/test_dir/dst1 > /dev/null || exit 1
ln -s ${DATADIR}/test_dir/src2 ${DATADIR}/dst2
if [ ! -L ${DATADIR}/dst2 ]; then
    exit 1
fi
cat ${DATADIR}/dst2 > /dev/null || exit 1
mv ${DATADIR}/test_dir ${DATADIR}/test_dir2 || exit 1
cat ${DATADIR}/test_dir/dst1 && exit 1
cat ${DATADIR}/dst2 && exit 1
rm ${DATADIR}/dst2 || exit 1
if [ -L ${DATADIR}/dst2 ]; then
    exit 1
fi
cat ${DATADIR}/test_dir2/src2 || exit 1
rm -r ${DATADIR}/test_dir2
if [ -f ${DATADIR}/test_dir2/src1 ] ||  [ -f ${DATADIR}/test_dir2/src2 ]; then
    exit 1
fi

echo "---- step 10 ---"

for i in $(seq 0 2000)
do
    echo $i >> ${DATADIR}/$i || exit 1
done
ls -al ${DATADIR} || exit 1
if [ "2001" != "$(ls ${DATADIR}/ | wc -l)" ]; then
    exit 1
fi
rm -rf ${DATADIR}/* || exit 1

# performance
echo "--- s3fuse performance ---"
./perf.sh ${ROOTDIR}

echo "[Done] test.sh"
date
