CFLAGS = -W -Wall -Wno-unused-result -g -std=gnu99 -D_GNU_SOURCE
DESTDIR := $(shell scripts/config.py install_dir)

OBJS = common.o ctl.o event.o fs.o log.o meta.o process.o users.o

all: septic build_isolate isolate.conf client rootfs

septic: config.h main.o $(OBJS)
	gcc $(LDFLAGS) -o $@ main.o $(OBJS)

client: config.h client.o $(OBJS)
	gcc $(LDFLAGS) -o $@ client.o $(OBJS)

main.o: main.c
	gcc $(CFLAGS) -c -o $@ $<

%.o: %.c %.h
	gcc $(CFLAGS) -c -o $@ $<

config.local:
	touch $@

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

rootfs: root/usr/bin/python3.6 root/usr/bin/wrapper.py

root/usr/bin/python3.6:
	scripts/build_rootfs

root/usr/bin/wrapper.py: python/wrapper.py
	install -m 644 $< $@

test:
	@tests/test.sh

septic_install: septic
	install -d $(DESTDIR)
	install septic $(DESTDIR)
	install isolate.conf $(DESTDIR)
	install -d $(DESTDIR)/python
	install -m 644 python/exporter.py $(DESTDIR)/python
	install -m 644 python/master.py $(DESTDIR)/python
	install -m 644 python/wrapper.py $(DESTDIR)/python

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

check_uid:
	@if [[ -z "$(UID)" ]]; then echo "Need to specify the UID"; exit 1; fi

meta:
	mkdir -p `scripts/config.py meta_dir`
	chown $(UID) `scripts/config.py meta_dir`
.PHONY: meta

run:
	mkdir -p `scripts/config.py run_dir`
	chown $(UID) `scripts/config.py run_dir`
.PHONY: run

mkdirs: check_uid meta run

isolate_clean:
	make -C isolate clean

clean: isolate_clean
	rm -f $(OBJS) config.h septic.service septic client client.o isolate.bin isolate.conf .test.*

distclean: clean
	rm -f config.local
	rm -rf root
	rm -rf meta
	rm -rf run
	rm -rf home
