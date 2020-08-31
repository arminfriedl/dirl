[![Build Status](https://drone.friedl.net/api/badges/playground/suckless-quark/status.svg)](https://drone.friedl.net/playground/suckless-quark)

This is my private tree of [quark](tools.suckless.org/quark/). Upstream can be
found at https://git.suckless.org/quark.

Quark is a small http server.

# Feature Patches

## Dirl: Customizable directory listing
[dirl](https://git.friedl.net/playground/suckless-quark/src/branch/dirlist) lets
you serve a fully customizable directory listing.

You can compile `dirl` from the `dirlist` branch, download a pre-compiled [musl
binary](https://dirlist.friedl.net/bin/suckless/quark/quark-dirl) or even pull a
pre-made [docker
image](https://hub.docker.com/repository/docker/arminfriedl/quark).

You can find an example deployment of [here](https://dirlist.friedl.net/). It
uses the default template just with a custom css. You can define your own
templates too for full customization. For details see the dirl
[README.md](https://git.friedl.net/playground/suckless-quark/src/branch/dirlist/README.md).

# Issues

## fork: Resource temporarily unavailable
When running [quark](http://tools.suckless.org/quark/) (#6606994) on my system
with `sudo ./quark -p 9763 -u <user> -g <group>` it dies with `./quark: fork:
Resource temporarily unavailable` at `fork()`.

Reason being that by default quark sets the RLIMIT_NPROC to 512 processes. When running as a non-exclusive user this limit is easily reached before even starting quark.

`resource-depletion-fix` contains a small forkbomb (`minibomb.c`) to simulate a user with > 512 processes. Compile it with `make minibomb`. When running the minibomb and quark with the same user quark fails.

The `resource-depletion-fix` branch contains a fix by setting the RLIMIT_NPROC only if the current system limit is lower than what would be set by quark. You can [download the patch](https://dirlist.friedl.net/suckless/quark/), or compile from the `resource-depletion-fix` branch.

Note that quark also has a `-n` parameter with which the max number of processes can be set as an alternative to this patch.

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
