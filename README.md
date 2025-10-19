# boros

boros is a Python library for asynchronous I/O based on the `io_uring` API.

It features a friendly API for writing concurrent applications with async/await
with a focus on performance and usability.

> [!WARNING]
>
> boros is currently in its early stages and not yet ready for practical use.
> Expect to encounter bugs, missing features, and API breakage anytime.

## System Requirements

boros targets Linux kernels starting from version 6.1 and newer.

The implementation makes an effort to be correct even when run on older
kernels, but fatal exceptions will be raised.

Likewise, we will support many of the more recent `io_uring` features, with
a documentation hint to the required kernel version. Attempting to use any
library features with an outdated kernel will also result in exceptions
being raised.

## Hacking

If you want to work on boros directly, it is recommended to set up a development
environment with [Meson](https://mesonbuild.com/), [Python 3.12+](https://www.python.org/),
and [uv](https://github.com/astral-sh/uv).

If you want [clangd](https://clangd.llvm.org/) language server support for the
C++ portions of the codebase, run the following command from the project root:

```shell
meson setup build
```

This will create the Meson build cache with a `compile_commands.json` file in
the `build/` directory, which is where clangd searches for it by default.

Next, you will want a virtual environment for development:

```sh
uv venv
source .venv/bin/activate
```

To reinstall the `boros` library to your venv when you want to test a change,
run the following command:

```sh
uv sync --reinstall-package boros
```

## License

<sup>
This library is licensed under the terms of the <a href="LICENSE">ISC License<\a>.
</sup>
