#! /bin/bash
echo "Joining linbs"
psp-ar ar -M <<EOM
    CREATE libM3D.a
	ADDLIB libAMG.a
    ADDLIB libosl.a
    ADDLIB libbulletpsp.a
    SAVE
    END
EOM
ranlib libM3D.a