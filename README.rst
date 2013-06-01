snes2usb
========

A SNES game controller to USB converter.

Now you can use the reliable, long-lasting SNES controller with your computer games and enjoy that retro-feeling!

.. image:: https://raw.github.com/chiva/snes2usb/gh-pages/images/gamepad-inside.jpg
.. image:: https://raw.github.com/chiva/snes2usb/gh-pages/images/detail.jpg

Instructions
------------

The converter is compatible with Windows, Linux and Mac OS, and becasuse it makes use of the standard USB HID protocol, no special drivers are needed, just plug & play.

If the game is not compatible with gamepads, you can use some of the gamepad to keyboard mappers, such as joytokey_ (Windows), qjoypad_ (Linux), enjoy_ (Mac OS).

.. _joytokey: http://www-en.jtksoft.net/
.. _qjoypad: http://qjoypad.sourceforge.net/
.. _enjoy: http://abstractable.net/enjoy/

Firmware
--------

Firmware has been developed using the latest V-USB and avr-gcc version available at the moment under Ubuntu.

The hex file the ATtiny45 requires to work is already compiled and stored inside the ``firmware`` folder, it's called ``main.hex``.

To download the firmware to the target, you should have at least ``avrdude`` installed in any platform, to install them under Ubuntu::

    $ sudo apt-get install avrdude

One of the cheapest programmers os the USBasp, but if you have a different one, please modify the avrdude line to suit your needs.

Now you can download the firmware to the target issuing::

    $ avrdude -c usbasp -p attiny45 -U flash:w:main.hex:i

PCB Board
---------

The board files under the ``schematic`` folder have been designed using DipTrace_. PDF with schematic and gerbers have been exported so there is no need to use the software, unless you want to modify them.

.. image:: https://raw.github.com/chiva/snes2usb/gh-pages/images/pcb-front.jpg
.. image:: https://raw.github.com/chiva/snes2usb/gh-pages/images/pcb-back.jpg
.. image:: https://raw.github.com/chiva/snes2usb/gh-pages/images/pcb-mounted.jpg

All parts are standard and common, they could be found on eBay at the time of the writing, that way, specialized distributors with high shipping costs can be avoided.

============  ========  =============  ========
Description   Value     Pad            Quantity
============  ========  =============  ========
uController   ATtiny45  SOIC           1
Resistor      68 Ohm    0805           2
Resistor      1.5 kOhm  0805           1
Capacitor     4.7 uF    0805           1
Diode                   DO-214AC(SMA)  2
USB Cable     1.5~2m                   1
============  ========  =============  ========

.. _DipTrace: http://www.diptrace.com/

Thanks to
---------

Based and improved from source code found at:
http://www.instructables.com/id/USB-SNES-Controller/

Special thanks to: andreq, timeblade0 and blackowaya.