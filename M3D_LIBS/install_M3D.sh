#!/bin/sh
[[ -z "${PSPDEV}" ]] && [[ -z "${@}" ]] && echo "Error: PSPDEV variable not set and/or no arguments given!" && exit

if [[ ! -z "${@}" ]] then
PSPSDKDIR=$@/psp/sdk
else
PSPSDKDIR=$PSPDEV/psp/sdk
fi

echo -n "Creating target dir: ${PSPSDKDIR}"
mkdir -p $PSPSDKDIR || exit " Failure!"
echo " Successful!"

echo -n "Creating release dir..."
mkdir -p release/include release/lib || exit " Failure!"
echo " Successful!"

cp -ar AMGLib/M3D.h release/include
cp -ar AMGLib/libAMG.a release/lib
cp -ar bullet-2.82-r2704/src/libbulletpsp.a release/lib
cp -ar oslibmodv2-master/libosl.a release/lib

cd release/lib && sh join.sh && cd ../..

echo -n "Installing libs..."
install -t $PSPSDKDIR/lib/ release/lib/libM3D.a && \
install -t $PSPSDKDIR/include/ release/include/M3D.h || exit " Failure!"
echo " Successful!"
