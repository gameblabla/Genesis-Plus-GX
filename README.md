Lame Genesis GX
===============

Here's a fork of Genesis Plus GX that is based on the very last GPLv2+ release of Genesis Plus GX
before they decided to switch to a non-commericial license.

The goal is to have a decent Genesis/Megadrive emulator that is fairly portable.

Issues that remain
==============

There are still leftovers of non-free code, all of which became either GPL or BSD in the upstream MAME source code.

However, it is unclear if this also covers old code. For example, any modifications to it might still be under the non-free code.

Musashi got re-released under the GPLv2+, however that code does not work as-is on Genesis Plus GX.

In fact, Genesis Plus GX never upgraded to the newer version probably due to issues over the GPL.


The z80 emulator also got relicensed too but again, it is unclear if this applies to the old code.

We need to switch the YM2612 code to the Nuked OPN2 one (or anything else suitable really).

However, my attempt mostly resulted in Nuked OPN2 not working properly. (Not playing at the proper pitch)


Outside of licenses, this one features no Sega CD emulation unlike upstream. (That was only introduced since Genesis Plus GX 1.7)

Some unlicensed games might also not work on it.

There are still some leftovers (unsigned) that have not switched to C99 datatypes yet.
