#!/bin/sh
#
# pages.sh - Show/record/compare UI pages
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

#
# We generate PPM because its easy, and store as PNG because it's
# space-efficient.
#


#
# Image comparison:
# https://stackoverflow.com/q/5132749/11496135
#

#
# Make PNG reproducible:
# https://superuser.com/a/1220703
#
reproducible="-define png:exclude-chunks=date,time"


PK=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA====


json()
{
	"$top/tools/accenc.py" /dev/stdin $PK >"$dir/_db" ||
	    { rm -f "$dir/_dn"; exit 1; }
}


page_inner()
{
	local erase=false
	local incremental=false
	local json=
	local title=true

	while [ "$1" ]; do
		case "$1" in
		-e)	erase=true
			shift;;
		-i)	incremental=true
			shift;;
		-j)	json=$2
			shift 2;;
		-n)	title=false
			shift;;
		*)	break;;
		esac
	done

	local name=$1
	shift

	if [ "$mode" = names ]; then
		$title && echo $name
		return
	fi

	# --- incremental command construction ---

	if $incremental; then
		eval set - "$last_args" '"$@"'
	fi
	last_args=
	for n in "$@"; do
		last_args="$last_args '$n'"
	done

	# --- test selection ---

	[ "$mode" = last ] || [ -z "$select" -o "$name" = "$select" ] || return

	$title && echo === $name ===
	found=true

	if [ ! -r "$dir/_db" ]; then
		if $erase; then
			python3 -c 'import sys
sys.stdout.buffer.write(b"\xff" * 1024 * 2048);' >"$dir/_db" || exit
		elif [ "$json" ]; then
			echo "$json" |
			    "$top/tools/accenc.py" /dev/stdin $PK >"$dir/_db" ||
			    exit
		else
			"$top/tools/accenc.py" "$top/accounts.json" $PK \
			    >"$dir/_db" || exit
		fi
	fi
	if $gdb; then
		if [ "$mode" = interact ]; then
			interact=interact
		else
			interact=
		fi
		gdb --args $top/sim $quiet -d "$dir/_db" -C "$@" $interact
		exit
	elif [ "$mode" = interact ]; then
		$top/sim $quiet -d "$dir/_db" -C "$@" interact
		exit
	else
		$top/sim $quiet -d "$dir/_db" -C "$@" "screen $dir/_tmp.ppm" ||
		    exit
	fi

	case "$mode" in
	last)	return;;
	show)	display "$dir/_tmp.ppm";;
	run)	;;
	test)	convert $reproducible "$dir/_tmp.ppm" "$dir/_tmp.png"
		[ "`md5sum <\"$dir/$name.png\"`" = \
		  "`md5sum <\"$dir/_tmp.png\"`" ] || {
			compare "$dir/$name.png" "$dir/_tmp.png" - |
			    display
			$all || exit
		}
		rm -f "$dir/_tmp.png";;
	store)	convert $reproducible "$dir/_tmp.ppm" "$dir/$name.png" || exit;;
	esac
}


cleanup()
{
	rm -f "$dir/_tmp.ppm" "$dir/_db"
}


page()
{
	page_inner "$@"
	local rc=$?
	cleanup
	return $rc
}


add()
{
	page -i "$@"
}


save()
{
	saved_args=$last_args
}


restore()
{
	last_args=$saved_args
}


usage()
{
	cat <<EOF 1>&2
usage: $0 [-a] [--gdb] [-v] [-x] [run|show|interact|last|test|store|names
          [page-name]]

-a  applies to "test": run all test cases and show the ones where differences
    are detected. Without -a, "test" stops after the first difference. The exit
    code is non-zero if the last test run produced a difference.
--gdb
    run the simulator under gdb
-v  enable debug output of the simulator
-x  set shell command tracing with set -x
EOF
	exit 1
}


self=`which "$0"`
dir=`dirname "$self"`
top=$dir/..

all=false
gdb=false
quiet=-q

while [ "$1" ]; do
	case "$1" in
	-a)	all=true
		shift;;
	--gdb)	gdb=true
		shift;;
	-v)	quiet=
		shift;;
	-x)	set -x
		shift;;
	-*)	usage;;
	*)	break;;
	esac
done

case "$1" in
run|show|test|store|last|names)
	mode=$1;;
inter|interact)
	mode=interact;;
"")	mode=test;;
*)	usage;;
esac

select=$2
found=false

rm -f "$dir/_db"

# --- on ----------------------------------------------------------------------

page on "random 1" button

# --- pin ---------------------------------------------------------------------

PIN_1="tap 123 71"
PIN_2="tap 49 135"
PIN_3="tap 130 252"
PIN_4="tap 125 135"
PIN_NEXT="tap 191 252"

page pin "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4"

# --- bad pin (once) ----------------------------------------------------------

page pin-bad1 "random 1" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT"

# --- bad pin (thrice) --------------------------------------------------------

page pin-bad3 "random 3" button \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "$PIN_2" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT"

# --- four seconds into cooldown  ---------------------------------------------

add cool-4s "tick 400"

# --- accounts ----------------------------------------------------------------

accounts()
{
	local opts=

	while [ "$1" ]; do
		case "$1" in
		-j)	opts="$opts -j $2"
			shift 2;;
		*)	break;;
		esac
	done

	local name=$1
	shift

	page $opts $name \
	    "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4" \
	    "$PIN_NEXT" "$@"
}


accounts accounts

# --- accounts scrolled up ----------------------------------------------------

accounts accounts-up "drag 158 243 159 117"

# --- account (demo)-----------------------------------------------------------

accounts account-demo "tap 86 67"

# --- account (HOTP hidden) ---------------------------------------------------

accounts account-hotp "drag 158 243 159 170" "tap 50 221"

# --- account (HOTP revealed)--------------------------------------------------

accounts account-hotp-reveal \
    "drag 158 243 159 170" "tap 50 221" "tap 38 80"

# --- account (TOTP)-----------------------------------------------------------

# Unix time 1716272769:
# UTC 2024-05-21 06:26:09
# Code 605617

accounts account-totp \
    "time 1716272769" "drag 158 243 159 130" "tap 41 241" tick

# --- accounts overlay (top) --------------------------------------------------

accounts accounts-over-top "long 201 23"

# --- accounts overlay (demo) --------------------------------------------------

accounts accounts-over-demo "long 45 69"

# ---  account overlay (demo) --------------------------------------------------

accounts account-demo-top-over "tap 86 67" "long 201 23"

# === accounts overlay add (demo) =============================================

ACCOUNTS_ADD="tap 60 169"
ACCOUNTS_MOVE="tap 120 168"
ACCOUNTS_CANCEL_MOVE="tap 193 177"
ACCOUNTS_REMOTE="tap 181 109"

accounts accounts-demo-add "long 45 69" "$ACCOUNTS_ADD"

# ---  accounts overlay add M, level 1 (demo) ---------------------------------

add accounts-demo-add-m1 "tap 200 136"

# ---  accounts overlay add M, level 2 (demo) ---------------------------------

add accounts-demo-add-m2 "tap 42 81"

# ---  accounts overlay add Me ------------------------------------------------

add accounts-demo-add-me "tap 194 83" "tap 119 137"

# ---  accounts added Me ------------------------------------------------------

add accounts-demo-added-me "tap 201 247"

# ---  account Me -------------------------------------------------------------

add account-me "tap 23 68"

# ---  account Me: fields list ------------------------------------------------

add account-me-fields "tap 119 165"

# ---  account Me: enter Password ---------------------------------------------

add account-me-pw "tap 71 167"

# ---  account Me: password Secr3t --------------------------------------------

ENTRY_1="tap 43 84"
ENTRY_2="tap 119 84"
ENTRY_3="tap 200 84"
ENTRY_4="tap 43 137"
ENTRY_5="tap 119 137"
ENTRY_6="tap 200 137"
ENTRY_7="tap 43 191"
ENTRY_8="tap 119 191"
ENTRY_9="tap 200 191"
ENTRY_L="tap 43 245"
ENTRY_0="tap 119 245"
ENTRY_R="tap 200 245"

add account-me-pw-secret \
    "$ENTRY_7" "$ENTRY_7" "$ENTRY_3" "$ENTRY_5" "$ENTRY_2" "$ENTRY_6" \
    "$ENTRY_7" "$ENTRY_6" "$ENTRY_3" "$ENTRY_0" "$ENTRY_8" "$ENTRY_4"

# ---  account Me: password added ---------------------------------------------

add account-me-pw-added "$ENTRY_R"

# --- setup -------------------------------------------------------------------

accounts setup "long 201 23" "tap 152 141"

# --- setup time --------------------------------------------------------------

accounts setup-time "time 1716272769" \
    "long 201 23" "tap 152 141" "tap 93 119"

# --- setup storage -----------------------------------------------------------

accounts setup-storage "long 201 23" "tap 152 141" "tap 81 214"

# --- delete account (swipe not started) --------------------------------------

accounts account-demo-top-delete "tap 86 67" "long 201 23" \
    "tap 193 142"

# --- delete account (yellow) -------------------------------------------------

accounts account-demo-top-delete-yellow "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 116 200"

# --- delete account (green) --------------------------------------------------

accounts account-demo-top-delete-green "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 180 200"

# --- deleted account ---------------------------------------------------------

accounts account-demo-deleted "tap 86 67" "long 201 23" \
    "tap 193 142" "drag 53 189 180 200"

# --- delete account (red) ---------------------------------------------------

accounts account-demo-top-delete-red "tap 86 67" "long 201 23" \
    "tap 193 142" "down 53 189" "move 180 150"

# --- account overlay (password field) ---------------------------------------

accounts account-demo-pw-over "tap 86 67" "long 199 119"

# --- delete field (swipe not started) ---------------------------------------

accounts account-demo-pw-delete "tap 86 67" "long 199 119" \
    "tap 153 174"

# --- deleted field  ----------------------------------------------------------

accounts account-demo-pw-deleted "tap 86 67" "long 199 119" \
    "tap 153 174" "drag 53 189 180 200"

# --- account overlay (bottom) ------------------------------------------------

accounts account-demo-bottom-over "tap 86 67" "long 107 194"

# --- account edit password (Geheimx) -----------------------------------------

# Adding an "x" to "Geheim" makes the text too long for centering, but doesn't
# yet require cutting any off-screen part.

accounts account-demo-pw-geheimx "tap 86 67" "long 199 119" \
    "tap 88 172" "$ENTRY_9" "$ENTRY_5"

# --- account show secret (HOTP) ----------------------------------------------

# The secret is a long base32 string that gets cut off at the screen edge.

accounts account-hotp-secret "drag 158 243 159 170" "tap 50 221" \
    "long 75 66" "tap 91 173"

# --- account with "comment" field --------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw",
    "comment":"comment" } ]
EOF

accounts account-comment "tap 86 67"

# --- account with 2nd password field -----------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts account-pw2 "tap 86 67"

# --- account with 2nd password and TOTP --------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp "time 0" "tap 86 67"

# --- account with 2nd password and TOTP, scrolled up, at 5 s -----------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp-up-5 \
    "time 5" "tap 86 67" "drag 158 243 159 130"

# --- account with 2nd password and TOTP, scrolled up, at 20 s ----------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp-up-20 \
    "time 20" "tap 86 67" "drag 158 243 159 130"

# --- account with 2nd password and TOTP, scrolled up, at 31 s ----------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2",
    "totp_secret": "GZ4FORKTNBVFGQTFJJGEIRDOKY======" } ]
EOF

accounts account-pw2-totp-up-31 \
    "time 31" "tap 86 67" "drag 158 243 159 130"

# --- delete 2nd password field -----------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts delete-pw2 "tap 86 67" \
    "long 119 233" "tap 154 173" "drag 56 190 183 191"

# --- delete 1st password field -----------------------------------------------

json <<EOF
[ { "id":"id", "user":"user", "email":"email", "pw":"pw", "pw2":"pw2" } ]
EOF

accounts delete-pw1 "tap 86 67" \
    "long 119 177" "tap 154 173" "drag 56 190 183 191"

# --- setup version -----------------------------------------------------------

accounts setup-version "long 201 23" "tap 152 141" static "tap 81 268"

# --- setup R&D ---------------------------------------------------------------

accounts setup-rd "long 201 23" "tap 152 141" \
    "drag 66 200 68 103" "tap 68 219"

# --- Move from ---------------------------------------------------------------

accounts move-from "long 200 72" "$ACCOUNTS_MOVE"

# --- Moving -----------------------------------------------------------------

add moving "long 114 166"

# --- Moved -------------------------------------------------------------------

save
add moved "$ACCOUNTS_MOVE"
restore

# --- Move cancel -------------------------------------------------------------

add move-cancel "$ACCOUNTS_CANCEL_MOVE"

# === Edit entry name (demo) =================================================

accounts entry-edit "tap 86 67" "long 201 23" "tap 116 141"

# --- Edit entry name (remove "emo") ------------------------------------------

add entry-edit-d "$ENTRY_L" "$ENTRY_L" "$ENTRY_L"

# --- Edit entry name ("dummy1", duplicate) -----------------------------------

add entry-edit-dummy1 \
    "$ENTRY_8" "$ENTRY_5" \
    "$ENTRY_6" "$ENTRY_4" "$ENTRY_6" "$ENTRY_4" \
    "$ENTRY_9" "$ENTRY_6" \
    "$ENTRY_1" "$ENTRY_0"

# --- Edit entry name ("dummy12", then back, duplicate) -----------------------

save
add entry-edit-dummy12 \
    "$ENTRY_2" "$ENTRY_0" \
    "$ENTRY_L"
restore

# --- Edit entry name ("dummy1xxx" ) ------------------------------------------

add entry-edit-dummy1xxx \
    "$ENTRY_9" "$ENTRY_5" "$ENTRY_9" "$ENTRY_5" "$ENTRY_9" "$ENTRY_5"

# --- Edit entry name ("dummy1xxx123456" ) ------------------------------------

add entry-edit-6 \
    "$ENTRY_1" "$ENTRY_0" "$ENTRY_2" "$ENTRY_0" "$ENTRY_3" "$ENTRY_0" \
    "$ENTRY_4" "$ENTRY_0" "$ENTRY_5" "$ENTRY_0" "$ENTRY_6" "$ENTRY_0"

# --- Edit entry name ("dummy1xxx1234567", maximum length) --------------------

add entry-edit-7 "$ENTRY_7" "$ENTRY_0"

# --- Edit entry name ("dummy1xxx1234567", no more) ---------------------------

add entry-edit-8 "$ENTRY_8" "$ENTRY_0"

# === Pin change, old PIN, empty ==============================================

accounts change-old \
    "long 201 23" "tap 152 141" "tap 86 70"

# --- Pin change, old PIN, cancel ---------------------------------------------

save
add change-old-cancel "$ENTRY_L"
restore

# --- Pin change, old PIN, first digit ----------------------------------------

add change-old-1 "$ENTRY_1"

# --- Pin change, invalid PIN  ------------------------------------------------

save
add change-invalid "$ENTRY_1" "$ENTRY_1" "$ENTRY_1" "$ENTRY_R"
restore

# --- Pin change, new PIN  ----------------------------------------------------

add change-new "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"

# --- Pin change, new PIN, cancel  --------------------------------------------

save
add change-new-cancel "$ENTRY_L"
restore

# --- Pin change, new PIN, first digit  ---------------------------------------

add change-new-first "$ENTRY_1"

# --- Pin change, same PIN  ---------------------------------------------------

save
add change-same "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"
restore

# --- Pin change, new PIN, all six digits  ------------------------------------

add change-new-all "$ENTRY_2" "$ENTRY_9" "$ENTRY_5" "$ENTRY_8" "$ENTRY_0"

# --- Pin change, confirm PIN -------------------------------------------------

add change-confirm "$ENTRY_R"

# --- Pin change, confirm PIN, cancel -----------------------------------------

save
add change-confirm-cancel "$ENTRY_L"
restore

# --- Pin change, match -------------------------------------------------------

save
add change-match \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_9" "$ENTRY_5" "$ENTRY_8" "$ENTRY_0" \
    "$ENTRY_R"
restore

# --- Pin change, mismatch ----------------------------------------------------

add change-mismatch \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_9" "$ENTRY_6" "$ENTRY_8" "$ENTRY_0" \
    "$ENTRY_R"

# --- Remote ------------------------------------------------------------------

accounts remote "long 200 72" "$ACCOUNTS_REMOTE"

# --- Remote (reveal) ---------------------------------------------------------

RDOP_REVEAL=05
FIELD_PASSWORD=05
NO="tap 69 256"
YES="tap 175 256"

accounts rmt-reveal "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_REVEAL demo $FIELD_PASSWORD"

# --- Remote (reveal, no) -----------------------------------------------------

accounts rmt-reveal-no "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_REVEAL demo $FIELD_PASSWORD" "$NO"

# --- Remote (reveal, yes) ----------------------------------------------------

accounts rmt-reveal-yes "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_REVEAL demo $FIELD_PASSWORD" "$YES"

# --- Remote (set time, advance by 30 seconds) --------------------------------

RDOP_SET_TIME=07
T_150000=1725462000
T_150030=`expr $T_150000 + 30`
T_120000=`expr $T_150000 - 3 \* 3600`
T_010000=`expr $T_150000 + 10 \* 3600`

t_bytes()
{
	local tmp=$1
	local i=0

	while [ $i -lt 8 ]; do
		[ "$i" -gt 0 ] && printf " "
		printf "%02x" `expr $tmp % 256`
		tmp=`expr $tmp / 256`
		i=`expr $i + 1`
	done
}

accounts rmt-set-time-30s "time $T_150000" \
    "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_SET_TIME `t_bytes $T_150030`"

# --- Remote (set time, back 3h) ----------------------------------------------

accounts rmt-set-time-3h "time $T_150000" \
    "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_SET_TIME `t_bytes $T_120000`"

# --- Remote (set time, advance by 10h) ---------------------------------------

accounts rmt-set-time-10h "time $T_150000" \
    "long 200 72" "$ACCOUNTS_REMOTE" \
    "rmt $RDOP_SET_TIME `t_bytes $T_010000`"

# --- account (HOTP revealed)--------------------------------------------------

saved_mode=$mode
mode=run
if ! page_inner -n hotp-reveal-twice \
    "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
    "drag 158 243 159 170" "tap 50 221" "tap 38 80"; then
	mode=$saved_mode
	cleanup
else
	mode=$saved_mode
	accounts hotp-reveal-twice \
	    "drag 158 243 159 170" "tap 50 221" "tap 38 80"
fi

# --- New device --------------------------------------------------------------

page -e new-on "random 1" button

# --- New device, cancel ------------------------------------------------------

page -e new-cancel "random 1" button \
    "$ENTRY_L"

# === New device, PIN =========================================================

page -e new-pin "random 1" button \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4"

# --- New device, confirm -----------------------------------------------------

add -e new-confirm "$ENTRY_R"

# --- New device, confirmed ---------------------------------------------------

add -e new-confirmed "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"

# --- New device, enter -------------------------------------------------------

add -e new-enter "$ENTRY_5"

# --- New device, mismatch ---------------------------------------------------

page -e new-mismatch "random 1" button \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R" \
    "$ENTRY_0" "$ENTRY_0" "$ENTRY_0" "$ENTRY_0" "$ENTRY_R"

# --- New device, repeat ---------------------------------------------------

add -e new-repeat "$ENTRY_5"

## --- accounts (empty) --------------------------------------------------------

accounts -j "[]" accounts-empty

# --- setup master secret -----------------------------------------------------

accounts setup-master "long 201 23" "tap 152 141" "tap 81 168"

# --- setup master secret, show pubkey ----------------------------------------

save
add setup-master-pubkey "tap 128 69"
restore

# --- setup master secret, PIN ------------------------------------------------

save
add setup-master-pin "tap 129 119"
restore

# --- setup master secret, show -----------------------------------------------

save
add setup-master-show "master scramble" "tap 129 119" \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"
restore

# --- setup master secret, show, back -----------------------------------------

add setup-master-show-back "master scramble" "tap 129 119" \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R" \
    "drag 200 100 80 100"

# === set master, PIN =========================================================

#
# Sections that have their title marked with === instead of --- contain
# incremental sequences (add ...). Be careful when making changes, as they may
# detail later tests !
#

set_master()
{
	local opts=

	while [ "$1" ]; do
		case "$1" in
		-j)	opts="$opts -j $2"
			shift 2;;
		*)	break;;
		esac
	done

	local name=$1
	shift

	page $opts $name \
	    "random 1" button "$PIN_1" "$PIN_2" "$PIN_3" "$PIN_4" "$PIN_NEXT" \
	    "long 201 23" "tap 152 141" "tap 81 168" "tap 71 172" "$@"
}

set_master sm-pin

# --- set-master, 1st, "paddle" -----------------------------------------------

add sm-1 "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"
add sm-1-p "$ENTRY_6"
add sm-1-pa "$ENTRY_1"
add sm-1-pad "$ENTRY_2"
add sm-1-padd "$ENTRY_2"

# --- set-master, 2nd, "pig" --------------------------------------------------

FIRST="tap 70 139"

add sm-2 "$FIRST"
add sm-2-p "$ENTRY_6"
add sm-2-pi "$ENTRY_4"
add sm-2-pig "$ENTRY_4"
add sm-2-pig-accept "$ENTRY_R"

# --- set-master, 3nd, "ship" -------------------------------------------------

add sm-3 "$FIRST"
add sm-3-s "$ENTRY_8"
add sm-3-sh "$ENTRY_4"
add sm-3-shi "$ENTRY_4"
add sm-3-ship "$ENTRY_6"

# --- set-master, 4th, "cat"  -------------------------------------------------

# "FIRST" here is actually the second entry, since the list moves up

add sm-4 "$FIRST"
add sm-4-cat "$ENTRY_2" "$ENTRY_1" "$ENTRY_9"
add sm-4-cat-accept "$ENTRY_R"

# --- set-master, 5th, "sword" ------------------------------------------------

# We already know after the 3rd input that the word is "sword"

add sm-5 "$FIRST"
add sm-5-s "$ENTRY_8"
add sm-5-sw "$ENTRY_0"
add sm-5-swo "$ENTRY_6"
#add sm-5-swo-accept "$FIRST"

# --- set-master, 6th to 24th -------------------------------------------------

SECOND="tap 28 166"

add sm-6-duck "$FIRST" "$ENTRY_2" "$ENTRY_9" "$ENTRY_2"
add sm-7-way "$FIRST" "$ENTRY_0" "$ENTRY_1" "$ENTRY_0" "$ENTRY_R"
add sm-8-wave "$FIRST" "$ENTRY_0" "$ENTRY_1" "$ENTRY_0" "$ENTRY_3"
add sm-9-abandon "$FIRST" "$ENTRY_1" "$ENTRY_1" "$ENTRY_1" "$ENTRY_6"
add sm-10-cluster "$FIRST" "$ENTRY_2" "$ENTRY_5" "$ENTRY_9" "$ENTRY_8"
add sm-11-effort "$FIRST" "$ENTRY_3" "$ENTRY_3" "$ENTRY_3" "$ENTRY_6"
add sm-12-giggle "$FIRST" "$ENTRY_4" "$ENTRY_4" "$ENTRY_4" "$ENTRY_4"
add sm-13-mammal "$FIRST" "$ENTRY_5" "$ENTRY_1" "$ENTRY_5" "$ENTRY_5"
add sm-14-oppose "$FIRST" "$ENTRY_6" "$ENTRY_6" "$ENTRY_6" "$ENTRY_6"
add sm-15-rural "$FIRST" "$ENTRY_7" "$ENTRY_9" "$ENTRY_7"

# Note: with "squeeze", the "U" could be auto-completed, but the user would
# then have to pay attention to any such auto-completion happening, which may
# not always be easy. So it's better to keep the system more predictable.
add sm-16-squeeze "$FIRST" "$ENTRY_8" "$ENTRY_7" "$ENTRY_9" "$ENTRY_3"

add sm-17-turtle "$FIRST" "$ENTRY_9" "$ENTRY_9" "$ENTRY_7" "$ENTRY_9"
add sm-18-wrong "$FIRST" "$ENTRY_0" "$ENTRY_7" "$ENTRY_6"
add sm-19-orient "$FIRST" "$ENTRY_6" "$ENTRY_7" "$ENTRY_4" "$ENTRY_3"
add sm-20-toe "$FIRST" "$ENTRY_9" "$ENTRY_6" "$ENTRY_3" "$ENTRY_R"
add sm-21-unfair "$FIRST" "$ENTRY_9" "$ENTRY_6" "$ENTRY_3" "$ENTRY_1"
add sm-22-unfold "$FIRST" "$ENTRY_9" "$ENTRY_6" "$ENTRY_3" "$ENTRY_6"
add sm-23-merit "$FIRST" "$ENTRY_5" "$ENTRY_3" "$ENTRY_7" "$ENTRY_4"
add sm-24-almost "$SECOND" "$ENTRY_1" "$ENTRY_5" "$ENTRY_5" "$ENTRY_6"

save
add sm-24-fail "$FIRST"
restore

add sm-24-done "$SECOND"

# bip39 decode paddle pig ship cat sword duck way wave abandon cluster effort giggle mammal oppose rural squeeze turtle wrong orient toe unfair unfold merit almost
# 9ed4931811edc487be07bf00058d1ab0f86b372f669ceaffd67271ced1da62e0

# --- set-master, verify ------------------------------------------------------

# ENTRY_5 is just a random tap

add sm-verify "$ENTRY_5" "tap 129 119" \
    "$ENTRY_1" "$ENTRY_2" "$ENTRY_3" "$ENTRY_4" "$ENTRY_R"

# === long entry, show ========================================================

accounts long-show "tap 32 270"

# --- long entry, scroll to the left ------------------------------------------

add long-left "drag 220 120 100 120"
add long-left2 "drag 220 120 110 121"
add long-left3 "drag 240 120 100 120"

# -----------------------------------------------------------------------------

if [ "$select" ] && ! $found; then
	echo "$select: not found" 1>&2
	exit 1
fi
[ "$1" = last ] && display "$dir/_tmp.ppm"
exit 0

#
# Tentative naming convention:
#
# - page: page which, together with an optional qualifier, uniquely defines
#   where and in what context we are, e.g., account, setup
#   Here, overlays don't count as pages;
# - page-qualifier: if there are different pages of a type, e.g., account-HOTP
# - item-on-page: if we reference a specific item on the page, e.g.,
#   accounts-demo-...
# - sub-page: if we invoke a sub-page on a main page, but are still in the
#   process of reaching a new page, e.g., accounts-top-over
#   If we are at an overlay, use "over". If we have progressed past an overlay,
#   use the option selected (or consider this a new page), e.g., "add" would be
#   somewhere > Overlay > Add, because what happens may depend on "somewhere",
#   but we reduce ... > Setup to just "setup", because it's the same
#   everywhere.
#   Can also be used to indicate a point in a sequence, e.g.,
#   accounts-demo-added-me, where we are interested in what the accounts list
#   looks like after having added "Me".
# - context at this point, e.g., m2 for text entry with the two letters "Me"
#
# This is probably too complicated. Revisit the naming issue later.
#
