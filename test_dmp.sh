#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Error: Please run this script with sudo"
    exit 1
fi

if ! lsmod | grep -q dmp; then
    echo "Error: dmp module is not loaded. Load it with 'sudo insmod dmp.ko'"
    exit 1
fi

echo "Creating temporary zero Device Mapper device..."
dmsetup create zero_test --table "0 204800 zero" || {
    echo "Error: Failed to create zero_test device"
    exit 1
}

echo "Creating test Device Mapper device with dmp target..."
dmsetup create dmp_test --table "0 204800 dmp /dev/mapper/zero_test"|| {
    echo "Error: Failed to create dmp_test device"
    dmsetup remove zero_test
    exit 1
}

echo "Test statistics before:"
cat /sys/module/dmp/stat/volumes || {
    echo "Error: Failed to read statistics"
    dmsetup remove dmp_test
    dmsetup remove zero_test
    exit 1
}

echo
echo "Performing test read (1 MB)..."
dd if=/dev/mapper/dmp_test of=/dev/null bs=1M count=1 status=none || {
    echo "Error: Read test failed"
    dmsetup remove dmp_test
    dmsetup remove zero_test
    exit 1
}
sync
sleep 1

echo "Test statistics:"
cat /sys/module/dmp/stat/volumes || {
    echo "Error: Failed to read statistics"
    dmsetup remove dmp_test
    dmsetup remove zero_test
    exit 1
}

echo
echo "Performing test write (1 MB)..."
dd if=/dev/random of=/dev/mapper/dmp_test bs=1M count=1 status=none || {
    echo "Error: Write test failed"
    dmsetup remove dmp_test
    dmsetup remove zero_test
    exit 1
}
sync
sleep 1

echo "Test statistics:"
cat /sys/module/dmp/stat/volumes || {
    echo "Error: Failed to read statistics"
    dmsetup remove dmp_test
    dmsetup remove zero_test
    exit 1
}

echo "Removing test device..."
dmsetup remove dmp_test || {
    echo "Error: Failed to remove dmp_test device"
    dmsetup remove zero_test
    exit 1
}

echo "Removing temporary zero device..."
dmsetup remove zero_test || {
    echo "Error: Failed to remove zero_test device"
    exit 1
}

echo "Test completed successfully"
exit 0