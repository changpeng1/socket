gcc client-publish-service.c -L/usr/lib/x86_64-linux-gnu -lavahi-client -lavahi-common


arm-poky-linux-gnueabi-gcc client-publish-service.c -L/root/arm/lib/avahi/usr/lib -lavahi-client -lavahi-core -lavahi-common -L/root/arm/lib/dbus/usr/lib -ldbus-1 -I/root/arm/lib/avahi/usr/include -o client-publish
