#!/usr/bin/env bash
# ==================================================================
# Grid Simulator — Linux launcher
# Checks serial port permissions before starting the app,
# and offers to fix them if needed.
# ==================================================================

APPDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APPBIN="$APPDIR/GridSimUserControl"

# Colour helpers
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
ok()    { echo -e "${GREEN}✓${NC} $*"; }
warn()  { echo -e "${YELLOW}⚠${NC} $*"; }
err()   { echo -e "${RED}✗${NC} $*"; }
info()  { echo -e "${CYAN}  $*${NC}"; }
ask()   { echo -e "${BOLD}$*${NC}"; }

echo -e "${BOLD}=================================================="
echo "  Grid Simulator — launch check"
echo -e "==================================================${NC}"

# ------------------------------------------------------------------
# 1. Make sure the binary exists and is executable
# ------------------------------------------------------------------
if [ ! -f "$APPBIN" ]; then
    err "Binary not found: $APPBIN"
    info "Run build_linux.sh first, or move this script next to GridSimUserControl."
    exit 1
fi
if [ ! -x "$APPBIN" ]; then
    warn "Binary not executable — fixing..."
    chmod +x "$APPBIN" || { err "Could not chmod $APPBIN"; exit 1; }
fi
ok "Binary: $APPBIN"

# ------------------------------------------------------------------
# 2. Display check (catches SSH sessions without X forwarding)
# ------------------------------------------------------------------
if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    warn "No display detected (\$DISPLAY and \$WAYLAND_DISPLAY are unset)."
    info "If you are connected via SSH, forward X11 with:  ssh -X user@host"
    info "Attempting to continue — the app may fail to start."
fi

# ------------------------------------------------------------------
# 3. Check dialout group membership
# ------------------------------------------------------------------
echo ""
echo "Checking serial port permissions..."

NEEDS_FIX=false
NEEDS_RELOGIN=false

if id -nG 2>/dev/null | tr ' ' '\n' | grep -qx "dialout"; then
    ok "User $(whoami) is in the 'dialout' group"
else
    warn "User $(whoami) is NOT in the 'dialout' group"
    info "Permanent fix: sudo usermod -aG dialout $(whoami)  then log out and back in"
    NEEDS_FIX=true
    NEEDS_RELOGIN=true
fi

# ------------------------------------------------------------------
# 4. Check actual serial devices
# ------------------------------------------------------------------
echo ""
FOUND_DEVS=()
BLOCKED_DEVS=()

for dev in /dev/ttyUSB* /dev/ttyACM*; do
    [ -e "$dev" ] || continue
    FOUND_DEVS+=("$dev")
    PERMS=$(stat -c "%A %U:%G" "$dev" 2>/dev/null || echo "?")
    if [ -r "$dev" ] && [ -w "$dev" ]; then
        ok "$dev  ($PERMS)  — accessible"
    else
        warn "$dev  ($PERMS)  — not accessible by $(whoami)"
        BLOCKED_DEVS+=("$dev")
        NEEDS_FIX=true
    fi
done

if [ ${#FOUND_DEVS[@]} -eq 0 ]; then
    warn "No USB serial devices found (/dev/ttyUSB* or /dev/ttyACM*)"
    info "Check that the NXP board is plugged in and recognised by the OS"
    info "Run 'dmesg | tail -20' to see recent USB events"
fi

# ------------------------------------------------------------------
# 5. Fix permissions if needed
# ------------------------------------------------------------------
if [ "$NEEDS_FIX" = true ]; then
    echo ""
    echo -e "${YELLOW}Serial permissions need attention.${NC}"
    echo ""
    ask "How would you like to proceed?"
    echo "  [1] Add $(whoami) to 'dialout' group permanently + fix devices now"
    echo "  [2] Fix device permissions for this session only (no persistent changes)"
    echo "  [3] Launch anyway without fixing (serial may fail to open)"
    echo "  [4] Quit"
    echo ""
    read -rp "Choice [1-4]: " CHOICE

    case "${CHOICE:-3}" in
        1)
            echo ""
            echo "Adding $(whoami) to the dialout group..."
            if sudo usermod -aG dialout "$(whoami)"; then
                ok "Added to 'dialout' group"
                warn "You must log out and log back in for this to take full effect."
                warn "For now, applying a temporary fix to devices in this session..."
            else
                warn "usermod failed — will try device-level fix instead"
            fi

            # Temporary device fix for this session
            FIXED=0
            for dev in "${BLOCKED_DEVS[@]}"; do
                if sudo chmod a+rw "$dev" 2>/dev/null; then
                    ok "Temporarily opened: $dev"
                    (( FIXED++ )) || true
                else
                    err "Could not open: $dev"
                fi
            done
            # Also fix any devices that appeared while we were running
            for dev in /dev/ttyUSB* /dev/ttyACM*; do
                [ -e "$dev" ] || continue
                sudo chmod a+rw "$dev" 2>/dev/null && (( FIXED++ )) || true
            done
            [ $FIXED -gt 0 ] && ok "Opened $FIXED device(s) for this session"
            ;;

        2)
            echo ""
            FIXED=0
            for dev in /dev/ttyUSB* /dev/ttyACM*; do
                [ -e "$dev" ] || continue
                if sudo chmod a+rw "$dev" 2>/dev/null; then
                    ok "Temporarily opened: $dev"
                    (( FIXED++ )) || true
                else
                    err "Could not open: $dev  (try running with sudo)"
                fi
            done
            [ $FIXED -eq 0 ] && warn "No devices were fixed — serial may not work"
            warn "This change is lost when the device is unplugged or the system reboots."
            info "For a permanent fix: sudo usermod -aG dialout $(whoami)  then log out"
            ;;

        3)
            warn "Launching without fixing serial permissions"
            info "You can still open the app and connect later if permissions change"
            ;;

        4)
            echo "Exiting."
            exit 0
            ;;

        *)
            warn "Unrecognised choice — launching without changes"
            ;;
    esac
fi

# ------------------------------------------------------------------
# 6. Set library paths for the bundled build
# ------------------------------------------------------------------
if [ -d "$APPDIR/lib" ]; then
    export LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

if [ -d "$APPDIR/plugins" ]; then
    export QT_PLUGIN_PATH="$APPDIR/plugins"
    export QT_QPA_PLATFORM_PLUGIN_PATH="$APPDIR/plugins/platforms"
fi

# qt.conf in the same directory handles this automatically for Qt6,
# but QT_PLUGIN_PATH ensures it works on Qt5 too.

# ------------------------------------------------------------------
# 7. Launch
# ------------------------------------------------------------------
echo ""
ok "Launching Grid Simulator..."
echo ""

exec "$APPBIN" "$@"
