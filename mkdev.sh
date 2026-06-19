#!/bin/sh
set -eu

MODPATH="/lib/modules/$(uname -r)/kernel/drivers/char"
DEVICE="esccp"
DEVNODE="escc"

if [ ! -f "./${DEVICE}.ko" ]; then
	echo "error: ${DEVICE}.ko not found in current directory" >&2
	exit 1
fi

mkdir -p "${MODPATH}"
cp -f "./${DEVICE}.ko" "${MODPATH}/${DEVICE}.ko"

# Prefer modprobe when the module is installed under /lib/modules.
if ! modprobe "${DEVICE}" 2>/dev/null; then
	insmod "./${DEVICE}.ko"
fi

major="$(awk "\$2==\"${DEVICE}\" {print \$1}" /proc/devices)"
if [ -z "${major}" ]; then
	echo "error: could not determine major number for ${DEVICE}" >&2
	exit 1
fi

i=0
while [ "${i}" -le 5 ]; do
	node="/dev/${DEVNODE}${i}"
	rm -f "${node}"
	mknod "${node}" c "${major}" "${i}"
	chmod 666 "${node}"
	i=$((i + 1))
done

echo "Created /dev/${DEVNODE}0-5 with major ${major}"
#$Id: mkdev.sh,v 1.6 2026/06/08 copilot Exp $
