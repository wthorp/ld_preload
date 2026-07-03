#!/bin/sh

set -eu

repo_root=/workspace
suite_name="${SMOKE_SUITE:-default}"
artifact_dir="$repo_root/smoke-artifacts/$suite_name"
fixture_bin="$repo_root/smoke/fixture"
fixture_src="$repo_root/smoke/fixture.c"
shared_object="$repo_root/ld_preload.so"

mkdir -p "$artifact_dir"
rm -f "$artifact_dir"/*

echo "==> building shared object"
go build -o "$shared_object" -buildmode=c-shared "$repo_root"

echo "==> building smoke fixture"
cc -Wall -Wextra -Werror -O2 -o "$fixture_bin" "$fixture_src"

run_case() {
    mode="$1"
    name="$2"
    preload="$3"
    shift
    shift
    shift

    log_prefix="$artifact_dir/${mode}.${name}"
    echo "==> smoke case: ${mode}.${name}"

    if [ "$preload" = "yes" ]; then
        set -- env LD_PRELOAD="$shared_object" "$@"
    fi

    if timeout 10s "$@" >"$log_prefix.stdout" 2>"$log_prefix.stderr"; then
        return 0
    fi

    echo "case failed: ${mode}.${name}"
    timeout 10s strace -ff -o "$log_prefix.strace" "$@" \
        >"$log_prefix.strace.stdout" 2>"$log_prefix.strace.stderr" || true
    echo "artifacts saved under $artifact_dir"
    return 1
}

run_pair() {
    name="$1"
    shift

    run_case plain "$name" no "$@"
    run_case preload "$name" yes "$@"
}

run_pair true /bin/true
run_pair ls /bin/ls /tmp
run_pair stat /usr/bin/stat /tmp
run_pair cat-hosts /bin/cat /etc/hosts
run_pair fixture-basic "$fixture_bin" basic
run_pair fixture-metadata "$fixture_bin" metadata
run_pair fixture-missing "$fixture_bin" missing
run_pair fixture-xattr "$fixture_bin" xattr

echo "smoke harness passed for $suite_name"
