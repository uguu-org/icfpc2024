#!/bin/bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
   echo "$0 {program}"
   exit 1
fi

PROG=$1
INPUT=$(mktemp)
EXPECTED=$(mktemp)
ACTUAL=$(mktemp)

function die
{
   echo "$1"
   rm -f "$INPUT" "$EXPECTED" "$ACTUAL"
   exit 1
}

function run_test
{
   echo "$2" > "$INPUT"
   echo "$3" > "$EXPECTED"

   "./$PROG" "$INPUT" > "$ACTUAL" || die "$1: Failed: $?"
   if ! ( diff -w "$EXPECTED" "$ACTUAL" ); then
      die "$1: Output mismatched"
   fi
}

run_test $LINENO 'U- I$' '-3'
run_test $LINENO 'U! T' 'F'
run_test $LINENO 'U! F' 'T'
run_test $LINENO 'U# S4%34' '15818151'
run_test $LINENO 'I!' '0'
run_test $LINENO 'U$ I!' '!="a"'
run_test $LINENO 'U$ I4%34' '4%34="test"'

run_test $LINENO 'B+ I# I$' '5'
run_test $LINENO 'B- I$ I#' '1'
run_test $LINENO 'B* I$ I#' '6'
run_test $LINENO 'B/ U- I( I#' '-3'
run_test $LINENO 'B% U- I( I#' '-1'
run_test $LINENO 'B< I$ I#' 'F'
run_test $LINENO 'B> I$ I#' 'T'
run_test $LINENO 'B= I$ I#' 'F'
run_test $LINENO 'B= I$ I$' 'T'
run_test $LINENO 'B= S$ S#' 'F'
run_test $LINENO 'B= S$ S$' 'T'
run_test $LINENO 'B= T T' 'T'
run_test $LINENO 'B= T F' 'F'
run_test $LINENO 'B= F T' 'F'
run_test $LINENO 'B= F F' 'T'
run_test $LINENO 'B| T F' 'T'
run_test $LINENO 'B& T F' 'F'
run_test $LINENO 'B. S4% S34' '4%34="test"'
run_test $LINENO 'BT I$ S4%34' '4%3="tes"'
run_test $LINENO 'BD I$ S4%34' '4="t"'

run_test $LINENO '? B> I# I$ S9%3 S./' './="no"'
run_test $LINENO '? B< I# I$ S9%3 S./' '9%3="yes"'

run_test $LINENO 'B$ L" B+ v" v" B* I$ I#' '12'
run_test $LINENO 'B$ L# B$ L" B+ v" v" B* I$ I# v8' '12'
run_test $LINENO 'B$ L$ I$ I#' '3'
run_test $LINENO 'B$ B$ L# L$ v# B. SB%,,/ S}Q/2,$_ IK' 'B%,,/}Q/2,$_="Hello World!"'

rm -f "$INPUT" "$EXPECTED" "$ACTUAL"
exit 0
