#!/usr/bin/python3
import configparser
import itertools
import os
import sys

class NoSectionConfigParser(configparser.ConfigParser):
    fake_section = '[DEFAULT]'

    def _read(self, fp, *args, **kwargs):
        return super()._read(itertools.chain((self.fake_section,), fp), *args, **kwargs)

    def read_dict(self, dictionary, *args, **kwargs):
        return super().read_dict({ self.fake_section: dictionary }, *args, **kwargs)

    def __getitem__(self, key):
        return super().__getitem__(self.fake_section)[key]

    def __iter__(self):
        return super().__getitem__(self.fake_section).__iter__()

config = NoSectionConfigParser(interpolation=configparser.ExtendedInterpolation())
config.read_dict({ 'cwd': os.getcwd() })
with open('config.defaults') as f:
    config.read_file(f)
config.read('config.local')

if __name__ == '__main__':
    for s in sys.argv[1:]:
        print(config[s])
