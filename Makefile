CFLAGS = -W -Wall -Wno-unused-result -g -std=gnu99 -D_GNU_SOURCE
DESTDIR ?= /opt/septic

OBJS = main.o

all: septic build_isolate isolate.conf

septic: config.h $(OBJS)
	gcc $(LDFLAGS) -o $@ $(OBJS)

main.o: main.c
	gcc $(CFLAGS) -c -o $@ $<

%.o: %.c %.h
	gcc $(CFLAGS) -c -o $@ $<

config.h: config.defaults
	scripts/build_config

isolate.conf: isolate/default.cf scripts/build_isolate_conf
	scripts/build_isolate_conf < $< > $@

scripts/build_isolate_conf: scripts/build_isolate_conf.c config.h
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<

isolate:
	make -C isolate isolate
.PHONY: isolate

isolate.bin: isolate/isolate
	cp -a isolate/isolate $@

build_isolate: isolate isolate.bin

rootfs: root/usr/bin/python3.6

root/usr/bin/python3.6:
	scripts/build_rootfs

septic_install: septic
	install -d $(DESTDIR)
	install septic $(DESTDIR)
	install isolate.conf $(DESTDIR)

isolate_install: build_isolate
	install isolate.bin $(DESTDIR)

rootfs_install: rootfs
	cp -a root $(DESTDIR)

systemd:
	install -m 644 septic.service /etc/systemd/system

install: septic_install isolate_install rootfs_install

isolate_clean:
	make -C isolate clean

clean: isolate_clean
	rm -f $(OBJS) septic scripts/build_isolate_conf isolate.bin isolate.conf

distclean: clean
	rm -f config.h
	rm -rf root
