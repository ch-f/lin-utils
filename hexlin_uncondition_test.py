#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause
# Copyright (C) 2024 hexDEV GmbH - https://hexdev.de
import can
import sys
import threading
import binascii


def send_message(interface, data):
    bus = can.interface.Bus(channel=interface, bustype='socketcan')
    msg = can.Message(arbitration_id=0x002, data=data, is_extended_id=False)
    try:
        bus.send(msg)
        checksum_type = "Classic"
        print(f"Message sent on {interface} [ID: 0x{msg.arbitration_id & 0xFF:02x}, Checksum: {checksum_type}]: {binascii.hexlify(bytearray(data)).decode('ascii')}")
    except can.CanError:
        print("Failed to send message")
        sys.exit(2)


def listen_for_messages(interface, expected_data, timeout=1):
    bus = can.interface.Bus(channel=interface, bustype='socketcan')
    print(f"Listening for messages on {interface}...")
    message = bus.recv(timeout)
    if message:
        print(f"Received message on {interface} [ID: 0x{message.arbitration_id & 0xFF:02x}]: {binascii.hexlify(message.data).decode('ascii')}")
        if message.data == bytearray(expected_data):
            print("Received expected data.")
        else:
            print("\033[91mData mismatch!\033[0m")
            sys.exit(3)
    else:
        print(f"No message received within {timeout} second(s).")
        sys.exit(4)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python test_can.py CDEV TESTER_DEV")
        sys.exit(1)

    CDEV, TESTER_DEV = sys.argv[1], sys.argv[2]
    test_data = [0xde, 0xad, 0xbe, 0xef]

    # Using threading to handle simultaneous send and listen
    listener_thread = threading.Thread(target=listen_for_messages,
                                       args=(CDEV, test_data))
    listener_thread.start()

    # Send a message after a short delay to ensure listener is ready
    listener_thread.join(0.5)  # Small delay to ensure the listener is ready
    send_message(TESTER_DEV, test_data)

    # Ensure the listener thread has finished
    listener_thread.join()

    sys.exit(0)
