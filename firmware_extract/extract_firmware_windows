#!/bin/bash

type 7z >/dev/null 2>&1 || { echo >&2 "This requires p7zip but it is not installed."; exit 1; }

shopt -s nullglob #Critical behaviour change variable.

pattern="GameCaptureSetup*.msi"
fileList=( ${pattern} )
filename=${fileList[0]}
[[ ! -z "${filename// }" ]] || { echo >&2 "Cannot find a GameCaptureSetup*.msi file."; exit 1; }

[[ ! -e Firmware.tgz ]] || { echo >&2 "Firmware.tgz file already exists. Delete to rerun."; exit 1; }


TEMPDIR=`mktemp -d -p .`
mkdir ${TEMPDIR}/gchd

7z e -o${TEMPDIR} ${filename} x86_yPushFile3.dll
7z e -o${TEMPDIR}/gchd -r ${TEMPDIR}/x86_yPushFile3.dll MB86H57_H58_IDLE MB86H57_H58_ENC_H MB86M01_ASSP_NSEC_IDLE MB86M01_ASSP_NSEC_ENC_H

rm -f ${TEMPDIR}/x86_yPushFile3.dll
pushd ${TEMPDIR} && tar cvzf ../Firmware.tgz gchd && popd

rm -r ${TEMPDIR}
