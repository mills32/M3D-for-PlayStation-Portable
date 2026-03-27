#!/bin/sh
echo -n "Joining libs..."
psp-ar ar -M <<EOM
    CREATE libM3D.a
	ADDLIB libAMG.a
    ADDLIB libosl.a
    ADDLIB libbulletpsp.a
    SAVE
    END
EOM
psp-ranlib libM3D.a || exit "Failure"
echo "Successful!"
