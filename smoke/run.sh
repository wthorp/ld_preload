#!/bin/sh

set -eu

repo_root=/workspace
suite_name="${SMOKE_SUITE:-default}"
artifact_dir="$repo_root/smoke-artifacts/$suite_name"
fixture_bin="$repo_root/smoke/fixture"
fixture_src="$repo_root/smoke/fixture.c"
shared_object="$repo_root/ld_preload.so"

mkdir -p "$artifact_dir"
rm -rf "$artifact_dir"/*

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

passthrough_root="$(mktemp -d "$artifact_dir/passthrough.XXXXXX")"
passthrough_from="$passthrough_root/from"
passthrough_to="$passthrough_root/to"
mkdir -p "$passthrough_from" "$passthrough_to"
run_case preload fixture-invalid-relative yes env \
    LD_PRELOAD_REWRITE_FROM=relative \
    LD_PRELOAD_REWRITE_TO="$passthrough_to" \
    "$fixture_bin" passthrough "$passthrough_from" "$passthrough_to"
run_case preload fixture-invalid-empty yes env \
    LD_PRELOAD_REWRITE_FROM= \
    LD_PRELOAD_REWRITE_TO="$passthrough_to" \
    "$fixture_bin" passthrough "$passthrough_from" "$passthrough_to"
long_to="/$(dd if=/dev/zero bs=1 count=5000 2>/dev/null | tr '\0' a)"
run_case preload fixture-invalid-long yes env \
    LD_PRELOAD_REWRITE_FROM="$passthrough_from" \
    LD_PRELOAD_REWRITE_TO="$long_to" \
    "$fixture_bin" passthrough "$passthrough_from" "$passthrough_to"

rewrite_root="$(mktemp -d "$artifact_dir/rewrite.XXXXXX")"
rewrite_from="$rewrite_root/from"
rewrite_to="$rewrite_root/to"
rewrite_sibling="$rewrite_root/from-sibling"
mkdir -p "$rewrite_from" "$rewrite_to" "$rewrite_sibling"
printf '%s' 'source guard' >"$rewrite_from/guard.txt"
run_case preload fixture-rewrite yes env \
    LD_PRELOAD_REWRITE_FROM="$rewrite_from" \
    LD_PRELOAD_REWRITE_TO="$rewrite_to" \
    "$fixture_bin" rewrite "$rewrite_from" "$rewrite_to" "$rewrite_sibling"
run_case preload fixture-failopen yes env \
    LD_PRELOAD_REWRITE_FROM="$rewrite_from" \
    LD_PRELOAD_REWRITE_TO="$rewrite_to" \
    "$fixture_bin" failopen

workload_root="$(mktemp -d "$artifact_dir/workload.XXXXXX")"
workload_from="$workload_root/from"
workload_to="$workload_root/to"
mkdir -p "$workload_from" "$workload_to"

run_case preload workload-mv yes env \
    LD_PRELOAD_REWRITE_FROM="$workload_from" \
    LD_PRELOAD_REWRITE_TO="$workload_to" \
    /bin/sh -ceu '
        cd "$1"
        mkdir -p src
        printf "%s" "mv payload" > src/move-me.txt
        /bin/mv src/move-me.txt src/moved.txt
        [ ! -e "$2/src/move-me.txt" ]
        [ -f "$2/src/moved.txt" ]
        [ "$(cat "$2/src/moved.txt")" = "mv payload" ]
    ' sh "$workload_from" "$workload_to"

run_case preload workload-tar yes env \
    LD_PRELOAD_REWRITE_FROM="$workload_from" \
    LD_PRELOAD_REWRITE_TO="$workload_to" \
    /bin/sh -ceu '
        cd "$1"
        mkdir -p archive-src archive-dst
        printf "%s" "tar payload" > archive-src/file.txt
        /bin/tar -C archive-src -cf archive.tar file.txt
        /bin/tar -C archive-dst -xf archive.tar
        [ -f "$2/archive-dst/file.txt" ]
        [ "$(cat "$2/archive-dst/file.txt")" = "tar payload" ]
    ' sh "$workload_from" "$workload_to"

fd_sem_from="$(mktemp -d "$artifact_dir/fd-semantics-from.XXXXXX")"
fd_sem_to="$(mktemp -d "$artifact_dir/fd-semantics-to.XXXXXX")"
mkdir -p "$fd_sem_from/src" "$fd_sem_to/src"
run_case preload fixture-fd-semantics yes env \
    LD_PRELOAD_REWRITE_FROM="$fd_sem_from" \
    LD_PRELOAD_REWRITE_TO="$fd_sem_to" \
    "$fixture_bin" fd-semantics "$fd_sem_from" "$fd_sem_to"

audit_log="$artifact_dir/preload.workload-mv.audit.strace"
echo "==> audit case: preload.workload-mv"
env LD_PRELOAD="$shared_object" \
    LD_PRELOAD_REWRITE_FROM="$workload_from" \
    LD_PRELOAD_REWRITE_TO="$workload_to" \
    strace -f -o "$audit_log" /bin/sh -ceu '
        mkdir -p "$1/audit" "$2/audit"
        printf "%s" "audit payload" > "$2/audit/input.txt"
        /bin/mv "$1/audit/input.txt" "$1/audit/output.txt"
        [ ! -e "$2/audit/input.txt" ]
        [ -f "$2/audit/output.txt" ]
    ' sh "$workload_from" "$workload_to"

echo "smoke harness passed for $suite_name"
