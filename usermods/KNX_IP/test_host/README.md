# Host tests (PC, no ESP32 needed)

Logic tests for the per-segment objects, start/stop dimming and the GA plan from the guide.
They compile the usermod against minimal stub headers (`stub/`) – no network, no hardware.

```sh
cd usermods/KNX_IP
for t in test_seg test_dim test_plan test_mem; do
  g++ -std=gnu++17 -I test_host/stub -I . test_host/$t.cpp \
      usermod_knx_ip_segments.cpp usermod_knx_ip_dimming.cpp esp-knx-ip.cpp \
      esp-knx-ip-conversion.cpp usermod_knx_ip.cpp -o /tmp/$t && /tmp/$t | grep -v '^\[KNX'
done
```
