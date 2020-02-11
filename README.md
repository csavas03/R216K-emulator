# R216K Emulator

Emulator for
[this TPT computer](https://powdertoy.co.uk/Browse/View.html?ID=2303519),
the manual for which is available
[here](https://trigraph.net/powdertoy/R216/manual.md).

## Building

Get python and pip first if you don't have them already. Python is availabile
[here](https://www.python.org/downloads/); once you have that, getting pip is
as simple as following the instructions
[here](https://pip.pypa.io/en/stable/installing/).

Then get meson and ninja via pip:
```sh
$ pip install meson ninja
```

Finally, configure a build directory with meson and let ninja do its thing:
```sh
$ meson build
$ cd build
$ ninja
```
