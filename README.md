[![Build Status](https://drone.friedl.net/api/badges/playground/suckless-quark/status.svg)](https://drone.friedl.net/playground/suckless-quark)

This is my private tree of [quark](tools.suckless.org/quark/). Upstream can be
found at https://git.suckless.org/quark.

Quark is a small http server.

# Issues

## fork: Resource temporarily unavailable
When running [quark](http://tools.suckless.org/quark/) (#6606994) on my system
with `sudo ./quark -p 9763 -u <user> -g <group>` it dies with `./quark: fork:
Resource temporarily unavailable` at `fork()`.

# Github Users
If you are visiting this repository on GitHub, you are on a mirror of
https://git.friedl.net/playground/suckless-quark. This mirror is regularily
updated with my other GitHub mirrors. In contrast to other projects I do not
intend to move this tree to GitHub in the future. It is meant to stay a
read-only mirror.

You are welcome to pull any changes from this repository into your quark tree.
If you want to contribute consider [contributing](http://suckless.org/hacking/)
directly to [upstream](http://suckless.org/community/).

If you still - for whichever reasons - want to contribute to my tree directly,
feel free to send patches to dev[at]friedl[dot]net. Alternatviely you can issue
a pull request on GitHub which will be cherry picked into my tree.
