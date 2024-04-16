/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/* Copyright (C) 2024 hexDEV GmbH - https://hexdev.de */
#include <stdio.h>
#include <stdint.h>

static const unsigned char id_parity_tbl[] = {
	0x80, 0xc0, 0x40, 0x00, 0xc0, 0x80, 0x00, 0x40,
	0x00, 0x40, 0xc0, 0x80, 0x40, 0x00, 0x80, 0xc0,
	0x40, 0x00, 0x80, 0xc0, 0x00, 0x40, 0xc0, 0x80,
	0xc0, 0x80, 0x00, 0x40, 0x80, 0xc0, 0x40, 0x00,
	0x00, 0x40, 0xc0, 0x80, 0x40, 0x00, 0x80, 0xc0,
	0x80, 0xc0, 0x40, 0x00, 0xc0, 0x80, 0x00, 0x40,
	0xc0, 0x80, 0x00, 0x40, 0x80, 0xc0, 0x40, 0x00,
	0x40, 0x00, 0x80, 0xc0, 0x00, 0x40, 0xc0, 0x80
};

#define LINBUS_ID_PID_MASK	0x3f
#define LINBUS_ID_FROM_PID(P)	((P) & LINBUS_ID_PID_MASK)
#define LINBUS_PID_FROM_ID(I)	(LINBUS_ID_FROM_PID(I) | id_parity_tbl[LINBUS_ID_FROM_PID(I)])

typedef uint8_t u8;
typedef unsigned int uint;

static u8 lin_ser_get_lin_checksum(u8 pid, u8 n, const u8 *bytes, int enhanced_fl) {
	uint csum = 0;

	// For enhanced checksum, add PID to the checksum
	if (enhanced_fl)
		csum += pid;

	for (int i = 0; i < n; i++) {
		csum += bytes[i];
		if (csum > 255)
			csum -= 255;
	}

	return (u8)(~csum & 0xff);
}

int main() {
	u8 id = 6;
	u8 bytes[] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

	u8 pid = LINBUS_PID_FROM_ID(id);
	u8 n_of_bytes = sizeof(bytes) / sizeof(bytes[0]);

	u8 checksum_enhanced = lin_ser_get_lin_checksum(pid, n_of_bytes, bytes, 1);
	u8 checksum_classic = lin_ser_get_lin_checksum(pid, n_of_bytes, bytes, 0);

	printf("LIN checksum_enhanced (id:%d pid:0x%02X, n_of_bytes=%d): 0x%02X\n", id, pid, n_of_bytes, checksum_enhanced);
	printf("LIN checksum_classic  (id:%d pid:0x%02X, n_of_bytes=%d): 0x%02X\n", id, pid, n_of_bytes, checksum_classic);

	return 0;
}
