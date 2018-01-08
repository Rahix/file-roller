file-roller-vpk
===============

file-roller with support for Valve Pak format

More information about file-roller in README-OLD

## Installation

### ArchLinux

For ArchLinux users, there is a [PKGBUILD](https://gist.github.com/Rahix/9350588bb1380f08b7335d3622de9e9c) available. (Currently outdated!)

### Others

```console
$ mkdir build
$ cd build
$ meson ..
$ ninja
$ ninja install
```

### VPK tool

Just installing this version of file-roller is not enough to make the vpk support work. For that the python vpk tool
is required. It can be installed using pip:

```console
$ sudo pip install vpk
```

## LICENSE

This program is released under the terms of the GNU General Public
License (GNU GPL) version 2 or greater.
You can find a copy of the license in the file COPYING.
