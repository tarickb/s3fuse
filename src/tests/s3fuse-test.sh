#!/bin/bash -xe

MOUNTDIR=$1
#以下の変数を変更するときは文字数を変えないように注意
ROOTDIR=${MOUNTDIR}/test2
DATADIR=${ROOTDIR}/data
MVDATADIR=${ROOTDIR}/mvdata
WORKDIR=$(pwd)

# マウントディレクトリがあること
if [ ! -d  ${MOUNTDIR} ]; then
    echo "not found mount dir (${MOUNTDIR})"
    exit 1
fi

# 事前処理
rm -rf ${ROOTDIR}
mkdir -p ${DATADIR}
mkdir -p ${MVDATADIR}

# touchしてechoして文字が読み取れるか
STEP="1"; echo "test ${STEP}" ; date
touch ${DATADIR}/test
echo -n "hoge" > ${DATADIR}/test
if [ "$(cat ${DATADIR}/test)" != "hoge" ]; then
   echo "error ${STEP}"
   exit 1
fi

# echoして追加かきこみできるか
STEP="2"; echo "test ${STEP}" ; date
echo -n "fuga" >> ${DATADIR}/test
if [ "$(cat ${DATADIR}/test)" != "hogefuga" ]; then
    echo "error ${STEP}"
    exit 1
fi

# rm してファイルが消えるか
STEP="3"; echo "test ${STEP}" ; date
rm -f ${DATADIR}/test
if [ -f ${DATADIR}/test ]; then
    echo "error ${STEP}"
    exit 1
fi

# touchせずにリダイレクトして文字が読み取れるか
STEP="4"; echo "test ${STEP}" ; date
echo -n "hoge" > ${DATADIR}/test
if [ "$(cat ${DATADIR}/test)" != "hoge" ]; then
    echo "error ${STEP}"
    exit 1
fi

# unlinkしてファイルが消えるか
STEP="5"; echo "test ${STEP}" ; date
unlink ${DATADIR}/test
if [ -f ${DATADIR}/test ]; then
    echo "error ${STEP}"
    exit 1
fi

# mvでrenameきるか
STEP="6"; echo "test ${STEP}" ; date
touch ${DATADIR}/test
mv ${DATADIR}/test ${DATADIR}/test2
if [ -f ${DATADIR}/test ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ ! -f ${DATADIR}/test2 ]; then
    echo "error ${STEP}-2"
    exit 1
fi

# mvでファイルを別のパスに移動できるか
STEP="7"; echo "test ${STEP}" ; date
mv ${DATADIR}/test2 ${MVDATADIR}/test
if [ -f ${DATADIR}/test2 ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ ! -f ${MVDATADIR}/test ]; then
    echo "error ${STEP}-2"
    exit 1
fi

# 複数回redirectで追記してちゃんとその行数あるか
STEP="8"; echo "test ${STEP}" ; date
for i in $(seq 1 100)
do
    echo "test message ${i}" >> ${DATADIR}/multi_line  || exit 1
done
if [ ! -f ${DATADIR}/multi_line ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ "100" -ne "$(cat ${DATADIR}/multi_line | wc -l)" ]; then
    echo "error ${STEP}-2"
    exit 1
fi

# copyできるか
STEP="9"; echo "test ${STEP}" ; date
cp ${DATADIR}/multi_line ${DATADIR}/multi_line2
if [ ! -f ${DATADIR}/multi_line ] || [ ! -f ${DATADIR}/multi_line2 ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ ! -z "$(diff ${DATADIR}/multi_line ${DATADIR}/multi_line2)" ]; then
    echo "error ${STEP}-2"
    exit 1
fi

# lsできるか
STEP="10"; echo "test ${STEP}" ; date
if [ "2" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi

# ファイルのchmodできるかどうか
STEP="11"; echo "test ${STEP}" ; date
chmod 666 ${MVDATADIR}/test
if [ "$(ls -l ${MVDATADIR}/test | awk '{ print $1 }')" != "-rw-rw-rw-" ]; then
    echo "error ${STEP}"
    exit 1
fi

# ファイルのchownできるかどうか
STEP="12"; echo "test ${STEP}" ; date
chown nobody. ${MVDATADIR}/test
if [ "$(ls -l ${MVDATADIR}/test | awk '{ print $3,$4 }')" != "nobody nobody" ]; then
    if [ "$(ls -l ${MVDATADIR}/test | awk '{ print $3,$4 }')" != "nobody nogroup" ]; then
        echo "error ${STEP}"
        exit 1
    fi
fi

# globでrmしてみる
STEP="13"; echo "test ${STEP}" ; date
rm -rf  ${DATADIR}/*
rm -rf  ${MVDATADIR}/*
ls ${DATADIR}
if [ -f ${DATADIR}/multi_line ] || [ -f ${DATADIR}/multi_line2 ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ -f ${MVDATADIR}/test ]; then
    echo "error ${STEP}-2"
    exit 1
fi
if [ "0" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}-3"
    exit 1
fi

# directoryを作る
STEP="14"; echo "test ${STEP}" ; date
mkdir -p ${DATADIR}/dir_test
if [ ! -d ${DATADIR}/dir_test ]; then
    echo "error ${STEP}"
    exit 1
fi

# ディレクトリのchmodできるかどうか
STEP="15"; echo "test ${STEP}" ; date
chmod 777 ${DATADIR}/dir_test
if [ "$(ls -al ${DATADIR}/dir_test | grep " \.$" | awk '{ print $1 }')" != "drwxrwxrwx" ]; then
    echo "error ${STEP}"
#    exit 1
fi

# ディレクトリのchownできるかどうか
STEP="16"; echo "test ${STEP}" ; date
chown nobody. ${DATADIR}/dir_test
if [ "$(ls -al ${DATADIR}/dir_test | grep " \.$" | awk '{ print $3,$4 }')" != "nobody nobody" ]; then
    if [ "$(ls -al ${DATADIR}/dir_test | grep " \.$" | awk '{ print $3,$4 }')" != "nobody nogroup" ]; then
        echo "error ${STEP}"
        exit 1
    fi
fi

# ディレクトリをmvする
STEP="17"; echo "test ${STEP}" ; date
mv ${DATADIR}/dir_test ${MVDATADIR}/dir_test
if [ -d ${DATADIR}/dir_test ]; then
     echo "error ${STEP}-1"
     exit 1
fi
if [ ! -d ${MVDATADIR}/dir_test ]; then
    echo "error ${STEP}-2"
    exit 1
fi

# ディレクトリを消す
STEP="18"; echo "test ${STEP}" ; date
rmdir ${MVDATADIR}/dir_test
if [ -d ${MVDATADIR}/dir_test ]; then
    echo "error ${STEP}"
    exit 1
fi

# 親のディレクトリから複数のディレクトリとファイルをまとめて消す
STEP="19"; echo "test ${STEP}" ; date
mkdir -p ${DATADIR}/tmp/a
mkdir -p ${DATADIR}/tmp/b
echo "test message" > ${DATADIR}/tmp/a/hoge
echo "test message" > ${DATADIR}/tmp/a/fuga
echo "test message" > ${DATADIR}/tmp/a/piyo
echo "test message" > ${DATADIR}/tmp/b/hoge
echo "test message" > ${DATADIR}/tmp/b/fuga
echo "test message" > ${DATADIR}/tmp/b/piyo
if [ ! -f ${DATADIR}/tmp/a/hoge ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/a/fuga ]; then
    echo "error ${STEP}-2"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/a/piyo ]; then
    echo "error ${STEP}-3"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/b/hoge ]; then
    echo "error ${STEP}-4"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/b/fuga ]; then
    echo "error ${STEP}-5"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/b/piyo ]; then
    echo "error ${STEP}-6"
    exit 1
fi
rm -rf  ${DATADIR}/tmp
if [ -d ${DATADIR}/tmp ]; then
    echo "error ${STEP}-7"
    exit 1
fi

# 親のディレクトリからまとめてコピー
STEP="20"; echo "test ${STEP}" ; date
mkdir -p ${DATADIR}/tmp/a
mkdir -p ${DATADIR}/tmp/b
echo "test message" > ${DATADIR}/tmp/a/hoge
echo "test message" > ${DATADIR}/tmp/a/fuga
echo "test message" > ${DATADIR}/tmp/a/piyo
echo "test message" > ${DATADIR}/tmp/b/hoge
echo "test message" > ${DATADIR}/tmp/b/fuga
echo "test message" > ${DATADIR}/tmp/b/piyo
if [ ! -f ${DATADIR}/tmp/a/hoge ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/a/fuga ]; then
    echo "error ${STEP}-2"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/a/piyo ]; then
    echo "error ${STEP}-3"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/b/hoge ]; then
    echo "error ${STEP}-4"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/b/fuga ]; then
    echo "error ${STEP}-5"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp/b/piyo ]; then
    echo "error ${STEP}-6"
    exit 1
fi
cp -r  ${DATADIR}/tmp ${DATADIR}/tmp2
if [ ! -d ${DATADIR}/tmp2 ]; then
    echo "error ${STEP}-7"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp2/a/hoge ]; then
    echo "error ${STEP}-8"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp2/a/fuga ]; then
    echo "error ${STEP}-9"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp2/a/piyo ]; then
    echo "error ${STEP}-10"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp2/b/hoge ]; then
    echo "error ${STEP}-11"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp2/b/fuga ]; then
    echo "error ${STEP}-12"
    exit 1
fi
if [ ! -f ${DATADIR}/tmp2/b/piyo ]; then
    echo "error ${STEP}-13"
    exit 1
fi
rm -rf ${DATADIR}/tmp
rm -rf ${DATADIR}/tmp2
if [ -d ${DATADIR}/tmp ]; then
    echo "error ${STEP}-14"
    exit 1
fi
if [ -d ${DATADIR}/tmp2 ]; then
    echo "error ${STEP}-15"
    exit 1
fi

# シンボリックリンクを作ってアクセスできるか
STEP="21"; echo "test ${STEP}" ; date
mkdir ${DATADIR}/sym_dir
echo -n "hoge" > ${DATADIR}/sym_dir/src1 || exit 1
echo -n "fuga" > ${DATADIR}/sym_dir/src2 || exit 1
if [ ! -f ${DATADIR}/sym_dir/src1 ] || [ ! -f ${DATADIR}/sym_dir/src2 ]; then
    echo "error ${STEP}-1"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir/src1)" != "hoge" ]; then
    echo "error ${STEP}-2"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir/src2)" != "fuga" ]; then
    echo "error ${STEP}-3"
    exit 1
fi

cd ${DATADIR}/sym_dir
ln -s ./src1 ./dst1 || exit 1
cd ${WORKDIR}
if [ ! -L ${DATADIR}/sym_dir/dst1 ]; then
    echo "error ${STEP}-4"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir/dst1)" != "hoge" ]; then
    echo "error ${STEP}-5"
    exit 1
fi
cd ${DATADIR}/sym_dir
ln -s ./src2 ./dst2
cd ${WORKDIR}
if [ ! -L ${DATADIR}/sym_dir/dst2 ]; then
    echo "error ${STEP}-6"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir/dst2)" != "fuga" ]; then
    echo "error ${STEP}-7"
    exit 1
fi
mv ${DATADIR}/sym_dir ${DATADIR}/sym_dir2
if [ -d ${DATADIR}/sym_dir ]; then
    echo "error ${STEP}-8"
    exit 1
fi
if [ ! -d ${DATADIR}/sym_dir2 ]; then
    echo "error ${STEP}-9"
    exit 1
fi
if [ ! -L ${DATADIR}/sym_dir2/dst1 ]; then
    echo "error ${STEP}-10"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir2/dst1)" != "hoge" ]; then
    echo "error ${STEP}-11"
    exit 1
fi
if [ ! -L ${DATADIR}/sym_dir2/dst2 ]; then
    echo "error ${STEP}-12"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir2/dst2)" != "fuga" ]; then
    echo "error ${STEP}-13"
    exit 1
fi
cd ${DATADIR}
ln -s ./sym_dir2 ./sym_dir3 || exit 1
cd ${WORKDIR}
if [ ! -L ${DATADIR}/sym_dir3/dst1 ]; then
    echo "error ${STEP}-14"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir3/dst1)" != "hoge" ]; then
    echo "error ${STEP}-15"
    exit 1
fi
if [ "$(cat ${DATADIR}/sym_dir3/dst2)" != "fuga" ]; then
    echo "error ${STEP}-16"
    exit 1
fi
rm -rf  ${DATADIR}/sym_dir2
rm -f  ${DATADIR}/sym_dir3
if [ -d ${DATADIR}/sym_dir2 ]; then
    echo "error ${STEP}-17"
    exit 1
fi
if [ -L ${DATADIR}/sym_dir3 ]; then
    echo "error ${STEP}-18"
    exit 1
fi

# リネーム
STEP="22"; echo "test ${STEP}"  ; date
touch ${DATADIR}/renametest
if [ ! -f ${DATADIR}/renametest ]; then
    echo "error ${STEP}-1"
    exit 1
fi
mv ${DATADIR}/renametest ${DATADIR}/hello
if [ -f ${DATADIR}/renametest ]; then
    echo "error ${STEP}-2"
    exit 1
fi
if [ ! -f ${DATADIR}/hello ]; then
    echo "error ${STEP}-3"
    exit 1
fi
rm -f  ${DATADIR}/hello
if [ -f ${DATADIR}/hello ]; then
    echo "error ${STEP}-4"
    exit 1
fi

# 掃除
rm -rf  ${DATADIR}/*

# 999ファイル
STEP="23"; echo "test ${STEP}"  ; date
for i in $(seq 1 999)
do
echo "test" > ${DATADIR}/${i}
done
if [ "999" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 1000ファイル
STEP="24"; echo "test ${STEP}"  ; date
for i in $(seq 1 1000)
do
echo "test" > ${DATADIR}/${i}
done
if [ "1000" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 1001ファイル
STEP="25"; echo "test ${STEP}"  ; date
for i in $(seq 1 1001)
do
echo "test" > ${DATADIR}/${i}
done
if [ "1001" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 999ディレクトリ
STEP="26"; echo "test ${STEP}"  ; date
for i in $(seq 1 999)
do
mkdir ${DATADIR}/${i}
done
if [ "999" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 1000ディレクトリ
STEP="27"; echo "test ${STEP}"  ; date
for i in $(seq 1 1000)
do
mkdir ${DATADIR}/${i}
done
if [ "1000" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 1001ディレクトリ
STEP="28"; echo "test ${STEP}"  ; date
for i in $(seq 1 1001)
do
mkdir ${DATADIR}/${i}
done
if [ "1001" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 500ファイル 499ディレクトリ
STEP="29"; echo "test ${STEP}"  ; date
for i in $(seq 1 500)
do
echo "test" > ${DATADIR}/f${i}
done
for i in $(seq 1 499)
do
mkdir ${DATADIR}/d${i}
done
if [ "999" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 500ファイル 500ディレクトリ
STEP="30"; echo "test ${STEP}"  ; date
for i in $(seq 1 500)
do
echo "test" > ${DATADIR}/f${i}
done
for i in $(seq 1 500)
do
mkdir ${DATADIR}/d${i}
done
if [ "1000" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 500ファイル 501ディレクトリ
STEP="31"; echo "test ${STEP}"  ; date
for i in $(seq 1 500)
do
echo "test" > ${DATADIR}/f${i}
done
for i in $(seq 1 501)
do
mkdir ${DATADIR}/d${i}
done
if [ "1001" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 499ファイル 500ディレクトリ
STEP="32"; echo "test ${STEP}" ; date
for i in $(seq 1 499)
do
echo "test" > ${DATADIR}/f${i}
done
for i in $(seq 1 500)
do
mkdir ${DATADIR}/d${i}
done
if [ "999" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 501ファイル 500ディレクトリ
STEP="33"; echo "test ${STEP}" ; date
for i in $(seq 1 501)
do
echo "test" > ${DATADIR}/f${i}
done
for i in $(seq 1 500)
do
mkdir ${DATADIR}/d${i}
done
if [ "1001" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 1001ファイル 1001ディレクトリ
STEP="34"; echo "test ${STEP}" ; date
for i in $(seq 1 1001)
do
echo "test" > ${DATADIR}/f${i}
done
for i in $(seq 1 1001)
do
mkdir ${DATADIR}/d${i}
done
if [ "2002" != $(ls ${DATADIR} | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 255バイトのディレクトリ1 / 255バイトのディレクトリ2 / 255バイトのディレクトリ3 / 170バイト(184 - len(test2/data/) - 3*len(/) = 184-3-11)のファイル名  len(test2/data/name) = 949
name=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 170 | head -n 1)
STEP="35"; echo "test ${STEP}"  ; date
cd ${DATADIR}
mkdir -p $(dirname ${name})
echo "hoge" > ${name}
cd ${WORKDIR}
if [ ! -f ${DATADIR}/${name} ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 255バイトのディレクトリ1 / 255バイトのディレクトリ2 / 255バイトのディレクトリ3 / 127バイトのディレクトリ4 / 43バイト(58 - 4*len(/) - len(test2/data/) = 58-4-11)のファイル名  len(/bucket_name/test2/data/) = 950 (仕様)
name=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 127 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 43 | head -n 1)
STEP="36"; echo "test ${STEP}" ; date
cd ${DATADIR}
mkdir -p $(dirname ${name})
echo "hoge" > ${name}
cd ${WORKDIR}
if [ ! -f ${DATADIR}/${name} ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 255バイトのディレクトリ1 / 255バイトのディレクトリ2 / 255バイトのディレクトリ3 / 127バイトのディレクトリ名 / 44バイト(59 - 4*len(/) - len(test2/data/) = 59-4-11)のファイル名  len(/bucket_name/test2/data/) = 951 (実装限界越え)
name=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 255 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 127 | head -n 1)/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w  44 | head -n 1)
STEP="37"; echo "test ${STEP}" ; date
cd ${DATADIR}
mkdir -p $(dirname ${name})
echo "hoge" > ${name} || echo "continue"
cd ${WORKDIR}
if [ -f ${DATADIR}/${name} ]; then
    echo "error ${STEP}"
    exit 1
fi
rm -rf  ${DATADIR}/*

# 複数プロセスでの書き込み
STEP="40"; echo "test ${STEP}"  ; date
(for i in $(seq 1 20) ; do echo "test message" > ${DATADIR}/1-${i}; done; touch ${DATADIR}/done-1) &
(for i in $(seq 1 20) ; do echo "test message" > ${DATADIR}/2-${i}; done; touch ${DATADIR}/done-2) &
(for i in $(seq 1 20) ; do echo "test message" > ${DATADIR}/3-${i}; done; touch ${DATADIR}/done-3) &
(for i in $(seq 1 20) ; do echo "test message" > ${DATADIR}/4-${i}; done; touch ${DATADIR}/done-4) &
(for i in $(seq 1 20) ; do echo "test message" > ${DATADIR}/5-${i}; done; touch ${DATADIR}/done-5) 
while [ ! -f ${DATADIR}/done-1 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/done-2 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/done-3 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/done-4 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/done-5 ];
do
   sleep 1
done
if [ "20" != $(ls ${DATADIR}/1-* | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
if [ "20" != $(ls ${DATADIR}/2-* | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
if [ "20" != $(ls ${DATADIR}/3-* | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
if [ "20" != $(ls ${DATADIR}/4-* | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
if [ "20" != $(ls ${DATADIR}/5-* | wc -l) ]; then
    echo "error ${STEP}"
    exit 1
fi
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/1-${i})" != "test message" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/2-${i})" != "test message" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/2-${i})" != "test message" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/3-${i})" != "test message" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/4-${i})" != "test message" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done

# 複数プロセスでの更新
STEP="41"; echo "test ${STEP}"  ; date
(for i in $(seq 1 20) ; do echo "test message2" > ${DATADIR}/1-${i}; done; touch ${DATADIR}/update_done-1) &
(for i in $(seq 1 20) ; do echo "test message2" > ${DATADIR}/2-${i}; done; touch ${DATADIR}/update_done-2) &
(for i in $(seq 1 20) ; do echo "test message2" > ${DATADIR}/3-${i}; done; touch ${DATADIR}/update_done-3) &
(for i in $(seq 1 20) ; do echo "test message2" > ${DATADIR}/4-${i}; done; touch ${DATADIR}/update_done-4) &
(for i in $(seq 1 20) ; do echo "test message2" > ${DATADIR}/5-${i}; done; touch ${DATADIR}/update_done-5) 
while [ ! -f ${DATADIR}/update_done-1 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/update_done-2 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/update_done-3 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/update_done-4 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/update_done-5 ];
do
   sleep 1
done
if [ "20" != $(ls ${DATADIR}/1-* | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "20" != $(ls ${DATADIR}/2-* | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "20" != $(ls ${DATADIR}/3-* | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "20" != $(ls ${DATADIR}/4-* | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "20" != $(ls ${DATADIR}/5-* | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/1-${i})" != "test message2" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/2-${i})" != "test message2" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/2-${i})" != "test message2" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/3-${i})" != "test message2" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done
for i in $(seq 1 20)
do
  if [ "$(cat ${DATADIR}/4-${i})" != "test message2" ]; then
    echo "error ${STEP}"
    exit 1
   fi
done

# 複数プロセスでの削除
STEP="42"; echo "test ${STEP}"  ; date
(for i in $(seq 1 20) ; do rm -f ${DATADIR}/1-${i}; done; touch ${DATADIR}/delete_done-1) &
(for i in $(seq 1 20) ; do rm -f ${DATADIR}/2-${i}; done; touch ${DATADIR}/delete_done-2) &
(for i in $(seq 1 20) ; do rm -f ${DATADIR}/3-${i}; done; touch ${DATADIR}/delete_done-3) &
(for i in $(seq 1 20) ; do rm -f ${DATADIR}/4-${i}; done; touch ${DATADIR}/delete_done-4) &
(for i in $(seq 1 20) ; do rm -f ${DATADIR}/5-${i}; done; touch ${DATADIR}/delete_done-5) 
while [ ! -f ${DATADIR}/delete_done-1 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/delete_done-2 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/delete_done-3 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/delete_done-4 ];
do
   sleep 1
done
while [ ! -f ${DATADIR}/delete_done-5 ];
do
   sleep 1
done
if [ "15" != $(ls ${DATADIR}/* | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "3" != $(ls ${DATADIR}/*-1 | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "3" != $(ls ${DATADIR}/*-2 | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "3" != $(ls ${DATADIR}/*-3 | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "3" != $(ls ${DATADIR}/*-4 | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "3" != $(ls ${DATADIR}/*-5 | wc -l) ]; then
   echo "error ${STEP}"
   exit 1
fi
rm -rf  ${DATADIR}/*

# 大きいファイルのマルチパートアップロード
STEP="43"; echo "test ${STEP}"  ; date
if [ ! -f /var/tmp/6G ]; then
    echo "create 6G file"
    dd if=/dev/urandom of=/var/tmp/6G bs=$((1024*1024)) count=$((6*1024))
fi
d1=$(sha512sum /var/tmp/6G | awk '{ print $1}')
cp /var/tmp/6G ${DATADIR}/6G
d2=$(sha512sum ${DATADIR}/6G | awk '{ print $1}')
if [ ! -f ${DATADIR}/6G ]; then
   echo "error ${STEP}"
   exit 1
fi
if [ "${d1}" != "${d2}" ]; then
   echo ${d1}
   echo ${d2}
   echo "error ${STEP}"
   exit 1
fi
rm -rf ${DATADIR}/*

echo "[Done] s3fuse-test.sh"
date
