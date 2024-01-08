#!/bin/bash
cat >gpg_config <<EOF
%echo Generating a basic OpenPGP key
Key-Type: RSA
Key-Length: 4096
Name-Real: zyx-test
Name-Email: xyz@f.bar
Expire-Date: 0
Passphrase:\n
%commit
%echo done
EOF

gpg --batch --passphrase='' --full-generate-key gpg_config
keys=$(gpg --list-secret-keys --keyid-format=long)
echo "output: $keys"