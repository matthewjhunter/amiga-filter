#!/bin/sh
# Compile Filter for AmigaOS with bebbo's m68k-amigaos-gcc, using the
# amigadev/crosstools Docker image (pinned by digest). Builds 68000, 68020
# and 68060 binaries into tests/build/. These are gcc/libnix builds, not
# SAS/C, and have not been run on real hardware. -Wno-pointer-sign
# because the NDK declares strings as unsigned char *.
set -eu

IMAGE="amigadev/crosstools:m68k-amigaos@sha256:3e19bf90d596c6d652a4381314562c6d1ac54f0f86b3c5007c7abd753a22fbb1"
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$here/build"

docker run --rm --network none \
	-v "$here/../src:/src:ro" \
	-v "$here/build:/out" \
	--user "$(id -u):$(id -g)" \
	"$IMAGE" sh -c '
		set -e
		for cpu in 000 020 060; do
			m68k-amigaos-gcc -noixemul -O2 -m68$cpu -Wall -Wno-main -Wno-pointer-sign \
				-o /out/Filter.$cpu \
				/src/Filter.c /src/user.c /src/messagebase.c
			echo "built Filter.$cpu"
		done
	'
