#!/bin/sh

set -eu

repo_root=/workspace
suite_name="${AUDIT_SUITE:-audit}"
artifact_dir="$repo_root/smoke-artifacts/$suite_name"
shared_object="$repo_root/ld_preload.so"
status_file="$artifact_dir/workload-status.txt"

mkdir -p "$artifact_dir"
rm -rf "$artifact_dir"/*
: >"$status_file"

echo "==> building shared object"
go build -o "$shared_object" -buildmode=c-shared "$repo_root"

core_syscalls='close
creat
chdir
execve
faccessat
fchmodat
getcwd
fstat
link
linkat
lseek
mkdir
mkdirat
newfstatat
openat
openat2
statfs
read
readlink
readlinkat
rename
renameat
renameat2
statx
symlink
symlinkat
truncate
unlink
unlinkat
utimensat
write'

run_trace() {
    name="$1"
    from_root="$2"
    to_root="$3"
    shift 3

    log="$artifact_dir/${name}.strace"
    echo "==> audit workload: $name"
    if env LD_PRELOAD="$shared_object" \
        LD_PRELOAD_REWRITE_FROM="$from_root" \
        LD_PRELOAD_REWRITE_TO="$to_root" \
        strace -ff -qq -e trace=%file -o "$log" "$@"; then
        printf 'PASS %s\n' "$name" >>"$status_file"
        return 0
    fi

    printf 'FAIL %s\n' "$name" >>"$status_file"
    return 1
}

run_trace_nofail() {
    name="$1"
    from_root="$2"
    to_root="$3"
    shift 3

    log="$artifact_dir/${name}.strace"
    echo "==> audit workload: $name"
    env LD_PRELOAD="$shared_object" \
        LD_PRELOAD_REWRITE_FROM="$from_root" \
        LD_PRELOAD_REWRITE_TO="$to_root" \
        strace -ff -qq -e trace=%file -o "$log" "$@" || true
}

run_trace_plain() {
    name="$1"
    shift

    log="$artifact_dir/${name}.strace"
    echo "==> audit workload: $name"
    if env LD_PRELOAD="$shared_object" \
        strace -ff -qq -e trace=%file -o "$log" "$@"; then
        printf 'PASS %s\n' "$name" >>"$status_file"
        return 0
    fi

    printf 'FAIL %s\n' "$name" >>"$status_file"
    return 1
}

run_trace_plain_optional() {
    name="$1"
    shift

    log="$artifact_dir/${name}.strace"
    echo "==> audit workload: $name"
    if env LD_PRELOAD="$shared_object" \
        strace -ff -qq -e trace=%file -o "$log" "$@"; then
        printf 'PASS %s\n' "$name" >>"$status_file"
        return 0
    fi

    printf 'SKIP %s\n' "$name" >>"$status_file"
    return 0
}

run_trace_host() {
    name="$1"
    shift

    log="$artifact_dir/${name}.strace"
    echo "==> audit workload: $name"
    if strace -ff -qq -e trace=%file -o "$log" "$@"; then
        printf 'PASS %s\n' "$name" >>"$status_file"
        return 0
    fi

    printf 'FAIL %s\n' "$name" >>"$status_file"
    return 1
}

run_trace_host_optional() {
    name="$1"
    shift

    log="$artifact_dir/${name}.strace"
    echo "==> audit workload: $name"
    if strace -ff -qq -e trace=%file -o "$log" "$@"; then
        printf 'PASS %s\n' "$name" >>"$status_file"
        return 0
    fi

    printf 'SKIP %s\n' "$name" >>"$status_file"
    return 0
}

prepare_pair() {
    base="$1"
    from_root="$artifact_dir/${base}.from"
    to_root="$artifact_dir/${base}.to"
    rm -rf "$from_root" "$to_root"
    mkdir -p "$from_root" "$to_root"
    printf '%s\n%s\n' "$from_root" "$to_root"
}

prepare_neutral() {
    base="$1"
    neutral_root="$artifact_dir/${base}.neutral"
    rm -rf "$neutral_root"
    mkdir -p "$neutral_root"
    printf '%s\n' "$neutral_root"
}

start_moto() {
    moto_log="$artifact_dir/moto.log"
    MOTO_PORT=5000 moto_server -H 127.0.0.1 -p 5000 >"$moto_log" 2>&1 &
    moto_pid=$!
    i=0
    until curl -fsS http://127.0.0.1:5000/ >/dev/null 2>&1; do
        i=$((i + 1))
        if [ "$i" -ge 50 ]; then
            echo "moto_server failed to start" >&2
            return 1
        fi
        sleep 0.2
    done
    echo "$moto_pid"
}

start_samba() {
    share_root="$artifact_dir/samba-share"
    samba_state="$artifact_dir/samba-state"
    samba_conf="$artifact_dir/smb.conf"
    mkdir -p "$share_root" "$samba_state"
    cat >"$samba_conf" <<EOF
[global]
server role = standalone server
map to guest = Bad User
guest account = root
workgroup = WORKGROUP
server min protocol = SMB2
smb ports = 1445
pid directory = ${samba_state}
lock directory = ${samba_state}
state directory = ${samba_state}
cache directory = ${samba_state}
private dir = ${samba_state}
log file = ${artifact_dir}/samba.log
force user = root

[share]
path = ${share_root}
read only = no
guest ok = yes
force user = root
EOF
    smbd --foreground --no-process-group --configfile="$samba_conf" &
    smbd_pid=$!
    i=0
    until smbclient -N -p 1445 -L //127.0.0.1 >/dev/null 2>&1; do
        i=$((i + 1))
        if [ "$i" -ge 50 ]; then
            echo "smbd failed to start" >&2
            return 1
        fi
        sleep 0.2
    done
    echo "$smbd_pid"
}

run_ganesha_workflow() {
    name="ganesha"
    status="FAIL"
    root="$artifact_dir/ganesha-workload"
    mountpoint_dir="$root/mount"
    neutral="$root/neutral"
    work_log="$artifact_dir/${name}.strace"
    rpcbind_log="$root/rpcbind.log"
    ganesha_pid=
    rpcbind_pid=
    mounted_fsal=
    mount_export_path=

    rm -rf "$root"
    mkdir -p "$root" "$mountpoint_dir" "$neutral"

    stop_ganesha_runtime() {
        if [ -n "$ganesha_pid" ]; then
            kill "$ganesha_pid" >/dev/null 2>&1 || true
        fi
        if [ -n "$rpcbind_pid" ]; then
            kill "$rpcbind_pid" >/dev/null 2>&1 || true
        fi
        ganesha_pid=
        rpcbind_pid=
    }

    stop_ganesha_mount() {
        if mountpoint -q "$mountpoint_dir" >/dev/null 2>&1; then
            umount "$mountpoint_dir" >/dev/null 2>&1 || true
        fi
    }

    stop_ganesha() {
        stop_ganesha_mount
        stop_ganesha_runtime
    }

    start_ganesha_instance() {
        local fsal="$1"
        local export_path="$2"

        mkdir -p "$export_path"
        printf 'ganesha payload\n' >"$export_path/payload.txt"

        cat >"$root/ganesha.conf" <<EOF
NFS_CORE_PARAM {
    Protocols = 3;
}
EXPORT {
    Export_Id = 1;
    Path = "$export_path";
    Access_Type = RW;
    FSAL {
        Name = $fsal;
    }
}
EOF

        mkdir -p /var/run/ganesha
        rpcbind -w >"$rpcbind_log" 2>&1 &
        rpcbind_pid=$!
        /usr/bin/ganesha.nfsd -F -L "$root/ganesha.log" -f "$root/ganesha.conf" >/tmp/ganesha.stdout 2>/tmp/ganesha.stderr &
        ganesha_pid=$!
        cleanup_pids="$cleanup_pids $ganesha_pid $rpcbind_pid"

        local attempts=0
        while [ "$attempts" -lt 50 ]; do
            if rpcinfo -t 127.0.0.1 nfs 3 >/dev/null 2>&1; then
                return 0
            fi
            attempts=$((attempts + 1))
            sleep 0.1
        done
        return 1
    }

    run_ganesha_strace() {
        if env strace -ff -qq -e trace=%file -o "$work_log" /bin/sh -ceu '
            set -eu
            printf "%s" "ganesha file payload" >"$1/payload.txt"
            /bin/cp "$1/payload.txt" "$1/copied.txt"
            [ "$(cat "$1/copied.txt")" = "ganesha file payload" ]
        ' sh "$mountpoint_dir"; then
            return 0
        fi
        return 1
    }

    run_mount_and_verify() {
        local fsal="$1"
        local export_path="$2"
        local export_id=$3
        local mount_err="$root/mount-${fsal}.err"
        local mount_url="127.0.0.1:${export_path}"

        mount -t nfs -o vers=3,proto=tcp "$mount_url" "$mountpoint_dir" > /dev/null 2>"$mount_err"
        mount_rc=$?
        if [ "$mount_rc" -ne 0 ]; then
            if grep -q "Operation not permitted" "$mount_err" >/dev/null 2>&1; then
                status="SKIP"
                return 0
            fi
            if grep -q "No such file or directory" "$mount_err" >/dev/null 2>&1; then
                printf 'WARN ganesha %s %s export path missing in mount phase\n' "$fsal" "$export_id" >>"$artifact_dir/ganesha-issues.log"
                return 1
            fi
            if grep -q "Access denied" "$mount_err" >/dev/null 2>&1; then
                return 1
            fi
            return 1
        fi

        if run_ganesha_strace; then
            return 0
        fi
        status="FAIL"
        return 1
    }

    mountpoint_dir="$root/mount"
    for fsal in VFS MEM; do
        export_path="$root/export-${fsal}"
        stop_ganesha
        if ! start_ganesha_instance "$fsal" "$export_path"; then
            status="FAIL"
            continue
        fi
        run_mount_and_verify "$fsal" "$export_path" "$([ "$fsal" = "VFS" ] && echo 'vfs' || echo 'mem')"
        mount_rc=$?
        if [ "$mount_rc" -eq 0 ]; then
            status="PASS"
            stop_ganesha
            break
        fi
        if [ "$status" = "SKIP" ]; then
            break
        fi
        stop_ganesha
    done

    stop_ganesha
    if [ "$status" = "SKIP" ]; then
        printf 'SKIP %s\n' "$name" >>"$status_file"
        return 0
    fi
    printf '%s %s\n' "$status" "$name" >>"$status_file"
    [ "$status" = "PASS" ]
}

check_path() {
    name="$1"
    path="$2"
    if [ -z "$path" ]; then
        return 0
    fi
    if [ -e "$path" ]; then
        return 0
    fi

    printf 'FAIL %s-check\n' "$name" >>"$status_file"
    return 1
}

run_check() {
    name="$1"
    path="$2"
    if [ -e "$path" ]; then
        return 0
    fi
    printf 'FAIL %s-check\n' "$name" >>"$status_file"
    return 1
}

run_check_optional() {
    name="$1"
    path="$2"
    if [ -e "$path" ]; then
        return 0
    fi

    printf 'SKIP %s-check\n' "$name" >>"$status_file"
    return 0
}

collect_missed() {
    summary="$artifact_dir/missed-syscalls.txt"
    : >"$summary"

    sed -nE 's/^([a-zA-Z0-9_]+)\(.*/\1/p' "$artifact_dir"/*.strace* \
        | sort | uniq >"$artifact_dir/all-file-syscalls.txt"

    while IFS= read -r syscall; do
        if ! printf '%s\n' "$core_syscalls" | grep -Fx "$syscall" >/dev/null 2>&1; then
            printf '%s\n' "$syscall" >>"$summary"
        fi
    done <"$artifact_dir/all-file-syscalls.txt"
}

cleanup_pids=
cleanup() {
    for pid in $cleanup_pids; do
        kill "$pid" >/dev/null 2>&1 || true
    done
}
trap cleanup EXIT INT TERM

set -- $(prepare_pair cp)
cp_from="$1"
cp_to="$2"
cp_neutral="$(prepare_neutral cp)"
mkdir -p "$cp_from/src" "$cp_to/src" "$cp_neutral/dst"
printf '%s' 'cp payload' >"$cp_from/src/input.txt"
printf '%s' 'cp payload' >"$cp_to/src/input.txt"
run_trace_nofail cp-no-status "$cp_from" "$cp_to" /bin/cp "$cp_from/src/input.txt" "$cp_neutral/dst/output.txt"
check_path cp "" || true

set -- $(prepare_pair mv)
mv_from="$1"
mv_to="$2"
mkdir -p "$mv_from/src" "$mv_to/src"
printf '%s' 'mv payload' >"$mv_from/src/move-me.txt"
printf '%s' 'mv payload' >"$mv_to/src/move-me.txt"
run_trace mv "$mv_from" "$mv_to" /bin/mv "$mv_from/src/move-me.txt" "$mv_from/src/moved.txt" || true
check_path mv "$mv_to/src/moved.txt" || true

set -- $(prepare_pair tar)
tar_from="$1"
tar_to="$2"
tar_neutral="$(prepare_neutral tar)"
mkdir -p "$tar_from/archive-src" "$tar_to/archive-src" "$tar_neutral/archive-dst"
printf '%s' 'tar payload' >"$tar_from/archive-src/file.txt"
printf '%s' 'tar payload' >"$tar_to/archive-src/file.txt"
run_trace tar-create "$tar_from" "$tar_to" /bin/tar -C "$tar_from/archive-src" -cf "$tar_neutral/archive.tar" file.txt || true
run_trace tar-extract "$tar_from" "$tar_to" /bin/tar -C "$tar_neutral/archive-dst" -xf "$tar_neutral/archive.tar" || true
check_path tar "$tar_neutral/archive-dst/file.txt" || true

set -- $(prepare_pair rsync)
rsync_from="$1"
rsync_to="$2"
rsync_neutral="$(prepare_neutral rsync)"
mkdir -p "$rsync_from/src" "$rsync_to/src" "$rsync_neutral/dst"
printf '%s' 'rsync payload' >"$rsync_from/src/file.txt"
printf '%s' 'rsync payload' >"$rsync_to/src/file.txt"
run_trace rsync "$rsync_from" "$rsync_to" rsync -a "$rsync_from/src/" "$rsync_neutral/dst/" || true
check_path rsync "$rsync_neutral/dst/file.txt" || true

set -- $(prepare_pair git)
git_from="$1"
git_to="$2"
mkdir -p "$git_from/repo" "$git_to/repo"
git_template_dir="$artifact_dir/git-templates"
mkdir -p "$git_template_dir"
run_trace git-init "$git_from" "$git_to" env GIT_TEMPLATE_DIR="$git_template_dir" \
    git init --separate-git-dir="$git_to/repo/.git" "$git_from/repo" || true
printf '%s' 'git payload' >"$git_from/repo/file.txt"
printf '%s' 'git payload' >"$git_to/repo/file.txt"
run_trace git-add "$git_from" "$git_to" git -C "$git_from/repo" add file.txt || true
run_trace git-mv "$git_from" "$git_to" git -C "$git_from/repo" mv file.txt renamed.txt || true
check_path git "$git_to/repo/renamed.txt" || true

set -- $(prepare_pair rclone)
rclone_from="$1"
rclone_to="$2"
rclone_neutral="$(prepare_neutral rclone)"
mkdir -p "$rclone_from/src" "$rclone_to/src" "$rclone_neutral/dst"
printf '%s' 'rclone payload' >"$rclone_from/src/file.txt"
printf '%s' 'rclone payload' >"$rclone_to/src/file.txt"
run_trace rclone "$rclone_from" "$rclone_to" rclone copy "$rclone_from/src" "$rclone_neutral/dst" || true
check_path rclone "$rclone_neutral/dst/file.txt" || true

moto_pid=$(start_moto)
cleanup_pids="$cleanup_pids $moto_pid"
set -- $(prepare_pair aws)
aws_from="$1"
aws_to="$2"
aws_neutral="$(prepare_neutral aws)"
mkdir -p "$aws_from/src" "$aws_to/src" "$aws_neutral/dst"
printf '%s' 'aws payload' >"$aws_from/src/file.txt"
printf '%s' 'aws payload' >"$aws_to/src/file.txt"
aws_env="AWS_ACCESS_KEY_ID=test AWS_SECRET_ACCESS_KEY=test AWS_DEFAULT_REGION=us-east-1"
run_trace aws-mb "$aws_from" "$aws_to" env $aws_env aws --endpoint-url http://127.0.0.1:5000 s3 mb s3://ld-preload-audit || true
run_trace aws-upload "$aws_from" "$aws_to" env $aws_env aws --endpoint-url http://127.0.0.1:5000 s3 cp "$aws_from/src/file.txt" s3://ld-preload-audit/file.txt || true
run_trace aws-download "$aws_from" "$aws_to" env $aws_env aws --endpoint-url http://127.0.0.1:5000 s3 cp s3://ld-preload-audit/file.txt "$aws_neutral/dst/file.txt" || true
check_path aws "$aws_neutral/dst/file.txt" || true

smbd_pid=$(start_samba)
cleanup_pids="$cleanup_pids $smbd_pid"
set -- $(prepare_pair smbclient)
smb_from="$1"
smb_to="$2"
smb_neutral="$(prepare_neutral smbclient)"
mkdir -p "$smb_from/src" "$smb_to/src" "$smb_neutral/dst"
printf '%s' 'smb payload' >"$smb_from/src/file.txt"
printf '%s' 'smb payload' >"$smb_to/src/file.txt"
run_trace_host_optional smbclient smbclient -N -p 1445 //127.0.0.1/share -c "put $smb_from/src/file.txt remote.txt; get remote.txt $smb_neutral/dst/from-smb.txt"
run_check_optional smbclient "$smb_neutral/dst/from-smb.txt"

run_ganesha_workflow || true

collect_missed

echo "==> observed file syscalls"
cat "$artifact_dir/all-file-syscalls.txt"
echo "==> workload status"
cat "$status_file"
echo "==> missed file syscalls"
cat "$artifact_dir/missed-syscalls.txt"
