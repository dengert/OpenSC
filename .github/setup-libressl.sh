#!/bin/bash

set -ex -o xtrace

# WARNING: Change this also in .github/workflows/linux.yml
V=libressl-4.0.0

sudo apt-get remove -y libssl-dev

if [ ! -d "$V" ]; then
	# letsencrypt CA does not seem to be included in CI runner
	wget --no-check-certificate https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/$V.tar.gz
	tar xzf $V.tar.gz

	pushd $V
#add patch here
patch << EOF
--- ./crypto/rsa/,rsa_pmeth.c	2025-01-15 09:47:34.808429739 -0600
+++ ./crypto/rsa/rsa_pmeth.c	2025-01-15 10:36:55.859764137 -0600
@@ -61,6 +61,10 @@
 #include <stdlib.h>
 #include <string.h>

+#include <execinfo.h>
+#include <stdio.h>
+#include <stdlib.h>
+
 #include <openssl/opensslconf.h>

 #include <openssl/asn1t.h>
@@ -670,6 +674,9 @@
 		else {
 			saltlen = strtonum(value, 0, INT_MAX, &errstr);
 			if (errstr != NULL) {
+				fprintf(stderr, "DEE sizeof(saltlen):%ld\n", sizeof(saltlen));
+				fprintf(stderr, "DEE strlen(value):%ld\n", strlen(value));
+				fprintf(stderr, "DEE error value:\"%s\" errstr:\"%s\"\n", value, (errstr)?errstr:"NULL");
 				RSAerror(RSA_R_INVALID_PSS_SALTLEN);
 				return -2;
 			}
EOF
	./configure --prefix=/usr/local
	make -j $(nproc)
	popd
fi

pushd $V
sudo make install
popd

# update dynamic linker to find the libraries in non-standard path
echo "/usr/local/lib64" | sudo tee /etc/ld.so.conf.d/openssl.conf
sudo ldconfig
