#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Headless boot tests for EOS.
#
# Boots eos.img under every emulation path real firmware might pick, reads the
# VGA text buffer straight out of guest memory over the QEMU monitor, and
# checks the shell actually came up. No display needed, so this works over ssh
# and in CI.
#
#   ./tests/smoke.sh            run everything
#   ./tests/smoke.sh floppy     run one case
# ---------------------------------------------------------------------------
set -uo pipefail
cd "$(dirname "$0")/.."

IMG=${IMG:-eos.img}
QEMU=${QEMU:-qemu-system-i386}
BOOT_WAIT=${BOOT_WAIT:-6}

[ -f "$IMG" ] || { echo "no $IMG - run make first"; exit 1; }
command -v "$QEMU" >/dev/null || { echo "$QEMU not installed"; exit 1; }

# --- read 0xB8000 out of the guest and turn it back into text --------------
screen() {                       # screen <qemu args...> [-- key key key]
    local args=() keys=()
    local seen_sep=0
    for a in "$@"; do
        if [ "$a" = "--" ]; then seen_sep=1; continue; fi
        if [ $seen_sep -eq 1 ]; then keys+=("$a"); else args+=("$a"); fi
    done

    {
        sleep "$BOOT_WAIT"
        for k in "${keys[@]:-}"; do [ -n "$k" ] && echo "sendkey $k" && sleep 0.15; done
        [ ${#keys[@]} -gt 0 ] && sleep 1
        echo "xp /1600xb 0xb8000"
        sleep 2
        echo "quit"
    } | timeout 60 "$QEMU" "${args[@]}" -display none -monitor stdio 2>/dev/null \
      | python3 -c '
import sys, re
b = []
for line in sys.stdin:
    m = re.match(r"^0{8}000b8[0-9a-f]{3}: (.*)", line.strip())
    if m:
        b += [int(x, 16) for x in m.group(1).split()]
txt = "".join(chr(b[i]) if 32 <= b[i] < 127 else " " for i in range(0, len(b), 2))
print("\n".join(txt[r:r+80].rstrip() for r in range(0, len(txt), 80)))'
}

pass=0; fail=0
check() {                        # check <name> <expected substring> <output>
    if grep -qF "$2" <<< "$3"; then
        printf '  \033[32mPASS\033[0m  %s\n' "$1"; pass=$((pass+1))
    else
        printf '  \033[31mFAIL\033[0m  %s (expected %q)\n' "$1" "$2"; fail=$((fail+1))
        sed 's/^/        | /' <<< "$(grep -v '^$' <<< "$3" | head -8)"
    fi
}

want=${1:-all}

if [ "$want" = all ] || [ "$want" = floppy ]; then
    echo "floppy  (USB-FDD emulation: DL=0x00, CHS reads, 18spt/2heads)"
    out=$(screen -drive "file=$IMG,format=raw,if=floppy" -boot a)
    check "boots to shell" "W3lC0M3 T0 EOS" "$out"
fi

if [ "$want" = all ] || [ "$want" = usb ]; then
    echo "usb     (USB mass storage, geometry decided by the BIOS)"
    out=$(screen -drive "if=none,id=stick,format=raw,file=$IMG" \
                 -usb -device usb-storage,drive=stick -boot c)
    check "boots to shell" "W3lC0M3 T0 EOS" "$out"
fi

if [ "$want" = all ] || [ "$want" = hdd ]; then
    echo "hdd     (USB-HDD emulation: DL=0x80, LBA reads via INT 13h AH=42h)"
    out=$(screen -drive "file=$IMG,format=raw,if=ide,index=0" -boot c)
    check "boots to shell" "W3lC0M3 T0 EOS" "$out"
fi

if [ "$want" = all ] || [ "$want" = shell ]; then
    echo "shell   (keyboard IRQ, parser, heap above 1 MB)"
    out=$(screen -drive "file=$IMG,format=raw,if=floppy" -boot a -- \
                 h e l p ret)
    check "help runs"       "Type shutdown to shutdown" "$out"

    out=$(screen -drive "file=$IMG,format=raw,if=floppy" -boot a -- \
                 c a l c spc 7 spc shift-8 spc 6 ret)
    check "calc 7 * 6 = 42" "42" "$out"

    out=$(screen -drive "file=$IMG,format=raw,if=floppy" -boot a -- \
                 m e m i n f o ret)
    check "allocator alive" "bytes" "$out"
fi

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
