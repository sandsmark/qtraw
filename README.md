[![pipeline status](https://gitlab.com/mardy/qtraw/badges/master/pipeline.svg)](https://gitlab.com/mardy/qtraw/commits/master)
[![coverage report](https://gitlab.com/mardy/qtraw/badges/master/coverage.svg)](https://gitlab.com/mardy/qtraw/commits/master)

QtRaw
=====

Qt image plugin for loading raw files, via [LibRaw](https://www.libraw.org/).
Once installed, it enables all Qt applications to load raw files produced by
most digital cameras.

The code is licensed under the GPL v3 licence. If you need to distribute it
under a different license, please contact me at info@mardy.it


Installation
------------

The QtRaw plugin depends on Qt and LibRaw (versions 0.15 till 0.19 are known to
be working). In order to build it, make sure you have the necessary development
packages installed. Under Ubuntu, this can be achieved by running these
commands:

    sudo apt-get install libraw-dev qtbase5-dev

for building with Qt 5, or

    sudo apt-get install libraw-dev libqt4-dev

for building with Qt 4.
Alternatively, you can find the LibRaw source code at
http://www.libraw.org/download and build it yourself.


Once the dependencies are set up, the following commands will build and install
the plugin into your system:

    qmake
    make
    sudo make install

If everything worked correctly, all Qt applications should now be able to load
and display raw camera files.
