#!/bin/bash
SOURCE_PATH=${SOURCE_PATH:-..}

source $SOURCE_PATH/tests/common.sh

echo "======================================================="
echo "Setup SoftHSM"
echo "======================================================="
if [[ ! -f $P11LIB ]]; then
    echo "WARNING: The SoftHSM is not installed. Can not run this test"
    exit 77;
fi

# The Ubuntu has old softhsm version not supporting this feature
grep "Ubuntu 18.04" /etc/issue && echo "WARNING: Not supported on Ubuntu 18.04" && exit 77

card_setup
assert $? "Failed to set up card"
# Test our PKCS #11 module here
# but we need a card to test some of the  tests i.e. slots
# P11LIB="../src/pkcs11/.libs/opensc-pkcs11.so"

echo "======================================================="
echo "Test pkcs11 threads IN "
echo "======================================================="
$PKCS11_TOOL --test-threads IN -L --module="$P11LIB"
assert $? "Failed running tests"


echo "======================================================="
echo "Test pkcs11 threads ILGISLT0 "
echo "======================================================="
$PKCS11_TOOL --test-threads ILGISLT0 -L --module="$P11LIB"
assert $? "Failed running tests"

exit $ERRORS
