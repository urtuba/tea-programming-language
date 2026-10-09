#!/bin/sh
# Run every test case in tests/cases against the program given as $1.
#
# A case is a program tests/cases/NAME.tea (or just NAME.args). It is run
# from inside tests/cases, with the path NAME.tea as its only argument.
# Optional files next to it:
#   NAME.args  the arguments to use instead of NAME.tea (may hold several
#              words, or be empty); it can also name a file that does not exist.
#              Standard input is NAME.tea if that file exists, so an empty
#              NAME.args runs the program on NAME.tea read from stdin.
#   NAME.out   expected stdout (required)
#   NAME.code  expected exit status (default 0)
#   NAME.err   expected stderr (checked only if the file exists)
#   NAME.errtail  expected last lines of stderr, for runs whose full trace
#              is too long to keep (checked only if the file exists)
#
# Every case that succeeds (exit status 0) and has no NAME.args is run a
# second time with NAME.tea on standard input and no arguments. It must give
# the same stdout and stderr.

bin=${1:?usage: run.sh PROGRAM}
bin=$(cd "$(dirname "$bin")" && pwd)/$(basename "$bin")
dir=$(cd "$(dirname "$0")" && pwd)/cases
tmp=$(mktemp -d "${TMPDIR:-/tmp}/tea-test.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' EXIT

# Compare one run of the program with the expected files of case $1.
# $2 is the file used as standard input, $3 the label shown in the result,
# the rest are the program's arguments.
check() {
    name=$1
    stdin_file=$2
    label=$3
    shift 3
    ok=1

    (cd "$dir" && "$bin" "$@" > "$tmp/out" 2> "$tmp/err" < "$stdin_file")
    status=$?

    expected_code=0
    if [ -f "$dir/$name.code" ]; then
        expected_code=$(cat "$dir/$name.code")
    fi

    if ! cmp -s "$tmp/out" "$dir/$name.out"; then
        echo "  stdout differs"
        ok=0
    fi
    if [ "$status" -ne "$expected_code" ]; then
        echo "  exit status $status, expected $expected_code"
        ok=0
    fi
    if [ -f "$dir/$name.err" ] && ! cmp -s "$tmp/err" "$dir/$name.err"; then
        echo "  stderr differs"
        ok=0
    fi
    if [ -f "$dir/$name.errtail" ]; then
        lines=$(wc -l < "$dir/$name.errtail")
        tail -n "$lines" "$tmp/err" > "$tmp/errtail"
        if ! cmp -s "$tmp/errtail" "$dir/$name.errtail"; then
            echo "  end of stderr differs"
            ok=0
        fi
    fi

    if [ "$ok" -eq 1 ]; then
        echo "PASS $label"
        pass=$((pass + 1))
    else
        echo "FAIL $label"
        fail=$((fail + 1))
    fi
}

pass=0
fail=0
for file in "$dir"/*.tea "$dir"/*.args; do
    [ -f "$file" ] || continue
    name=$(basename "${file%.*}")
    case $file in
        *.args) [ -f "$dir/$name.tea" ] && continue ;;
    esac

    case_input=/dev/null
    [ -f "$dir/$name.tea" ] && case_input=$dir/$name.tea

    if [ -f "$dir/$name.args" ]; then
        # Unquoted on purpose: the file holds the words to pass.
        # shellcheck disable=SC2046
        check "$name" "$case_input" "$name" $(cat "$dir/$name.args")
    else
        check "$name" /dev/null "$name" "$name.tea"
        if [ ! -f "$dir/$name.code" ] || [ "$(cat "$dir/$name.code")" -eq 0 ]; then
            check "$name" "$case_input" "$name (stdin)"
        fi
    fi
done

echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
