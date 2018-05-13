CFLAGS = -W -Wall -Wno-unused-result -g -std=gnu99 -D_GNU_SOURCE
DESTDIR := $(shell scripts/config.py install_dir)

OBJS = common.o log.o main.o

all: septic build_isolate isolate.conf

septic: config.h $(OBJS)
	gcc $(LDFLAGS) -o $@ $(OBJS)

main.o: main.c
	gcc $(CFLAGS) -c -o $@ $<

%.o: %.c %.h
	gcc $(CFLAGS) -c -o $@ $<

config.local:

config.h: config.defaults config.local
	scripts/build_config_h > $@

isolate.conf: isolate/default.cf
	scripts/build_isolate_conf $< > $@

isolate:
	make -C isolate isolate CONFIG=$(DESTDIR)/isolate.conf
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
	install -m 4755 isolate.bin $(DESTDIR)

rootfs_install: rootfs
	cp -a root $(DESTDIR)

septic.service: septic.service.in
	scripts/apply_config < $< > $@

systemd: septic.service
	install -m 644 septic.service /etc/systemd/system

install: septic_install isolate_install rootfs_install

rights: isolate.bin
	chown root:root isolate.bin
	chmod 4755 isolate.bin

isolate_clean:
	make -C isolate clean

clean: isolate_clean
	rm -f $(OBJS) config.h septic.service septic isolate.bin isolate.conf

distclean: clean
	rm -f config.h
	rm -rf root
