[![Build Status](https://drone.friedl.net/api/badges/incubator/dirl/status.svg)](https://drone.friedl.net/incubator/dirl)

Dirl is a tiny server for customized directory listings. It is based on
[quark](tools.suckless.org/quark/).

# Getting Started
Pre-built binaries linked against glibc and musl can be downloaded from
[https://dirlist.friedl.net](https://dirlist.friedl.net/bin/dirl).

In addition alpine based docker images are provided at
[DockerHub](https://hub.docker.com/repository/docker/arminfriedl/dirl).

To start a directory listing just run the binary or start a container.

# Defaults
Per default dirl generates standard compliant html for a directory listing like
this:

```html
<!DOCTYPE HTML PUBLIC " - // W3C//DTD HTML 3.2 Final//EN">
<!-- Header Section -->
<html>
  <head>
    <link rel="stylesheet" href="style.css">
    <title>Index of {uri}</title>
  </head>
  <body>
    <h1>Index of {uri}</h1>
    <p><a href="..">&crarr; Parent Directory</a></p>
    <hr />
    <table>
      <tr><th>Name</th><th>Modified</th><th>Size</th></tr>
<!-- /Header Section -->

<!-- Entry Section -->
<!-- (repeated for each entry in the directory) -->
      <tr>
        <td><a href="{entry}">{entry}{suffix}</a>
        <td>{modified}</td>
        <td>{size}</td>
      </tr>
<!-- /Entry Section -->

<!-- Footer Section -->
    </table>
    <hr />
    <p>
      Served by <a href="http://tools.suckless.org/quark/">quark</a> and <a href="https://git.friedl.net/playground/suckless-quark/src/branch/dirlist">dirl</a>
    </p>
</body>
</html>
<!-- /Footer Section -->
```

# Customization

The default listing can be styled by a `style.css` in the root directory.

You can also use your fully customized template by creating one or all the
template files for each section. Per default the section templates are named:
- .header.tpl
- .entry.tpl (repeated for each directory entry)
- .footer.tpl

Note that if you only provide some of the template files, they have to be
compatible with the generated default for the other sections.

For each of these templates you can use placeholders that are replaced by their respective values:
- header
    * `{uri}`: Replaced by the current path
- entry
    * `{entry}`: Name of the entry
    * `{suffix}`: A suffix for the entry, mostly useful to distinguish directories (suffix '/') from files
    * `{modified}`: Date the entry was last modified
    * `{size}`: Size of the entry (if available)
    
## Subdirectory styling

dirl tries to the closest template for the currently visited path. This gives
you the opportunity to override templates in subdirectories. dirl walks the
directory hierarchy upwards from the currently visited path. As soon as it finds
one of the template files in a directory, it stops searching and uses the
templates in that directory.

In case no templates are found up until and including root, the default
templates are used.

## Customize names

The files defined as templates and style are ignored in the directory listing
itself. In case you need to list one of these directories, or have any other
reason to choose different names, the filenames can be configured in `dirl.h`.
Note that you need to compile your own quark version then.

# Download
You can also download CI builds for [quark-dirl](https://dirlist.friedl.net/bin/suckless/quark/). 

There are no official releases. Quark has no dependencies and you can easily
build it from source. Don't forget to read up on the [suckless
philosophy](http://suckless.org/philosophy/).

# Github Users
If you are visiting this repository on GitHub, you are on a mirror of
https://git.friedl.net/incubator/dirl. This mirror is regularily updated with my
other GitHub mirrors. 

Like with my other incubator projects, once I consider `dirl` reasonable stable
the main tree will move on GitHub.

Dirl is built upon [quark](tools.suckless.org/quark/). You are welcome to pull
any changes from this repository into your own quark tree. If you want to
contribute consider [contributing](http://suckless.org/hacking/) directly to
[upstream](http://suckless.org/community/).

If you want to contribute to `dirl` directly, feel free to send patches to
dev[at]friedl[dot]net. Alternatviely, you can issue a pull request on GitHub
which will be cherry picked into my tree. If you plan significant long-term
contributions drop me a mail for access to the incubator repository.
