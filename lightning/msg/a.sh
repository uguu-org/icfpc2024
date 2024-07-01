#!/bin/bash
# Post a message stored in "send" directory over the communication channel,
# and store the reply in "recv" directory.

set -euo pipefail

if [[ $# -ne 1 ]]; then
   echo "$0 {id}"
   exit 1
fi
ID=$1
INPUT="send/${ID}.txt"
OUTPUT="recv/${ID}.txt"

cd $(dirname "$0")/


if ! [[ -s "$INPUT" ]]; then
   echo "$INPUT not found"
   exit 1
fi

if [[ -s "$OUTPUT" ]]; then
   cat "$OUTPUT"
   echo ""
   exit 0
fi

curl -s --header "Authorization: Bearer 358703d3-2b62-446a-817a-15f72b78bd88" -d @"$INPUT" -o "$OUTPUT" https://boundvariable.space/communicate

cat "$OUTPUT"
echo ""
exit 0
