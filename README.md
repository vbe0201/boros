# boros

boros is a Python library for asynchronous I/O based on the `io_uring` API.

It features a friendly API for writing concurrent applications based on
async/await with a focus on performance and usability.

> [!WARNING]
>
> boros is currently in its early stages and not yet ready for practical use.
> Expect to encounter bugs, missing features, and API breakage anytime.

## System Requirements

boros targets Linux kernels starting from version 6.1 and newer.

The implementation makes an effort to be correct even when run on older
kernels, but fatal exceptions will be raised.

Likewise, we will support many of the more recent `io_uring` features with
a documentation hint to the required kernel version. Attempting to use any
library features with an outdated kernel will also result in exceptions
being raised.

## Development

It is recommended to set up a developemnt environment with [Meson](https://mesonbuild.com),
[Python 3.12+](https://www.python.org), and [uv](https://docs.astral.sh/uv).

[just](https://github.com/casey/just) is used as a command runner for
convenience, it is recommended but not strictly required.

### Quick Start

Create a virtual environment for development and install a debug build
of boros to it:

```bash
uv venv

# Defaults to debug builds unless the BOROS_BUILD_MODE
# environment variable is set to release.
just sync
```

And you should be all set! Use commands like `just lint`, `just format`,
`just ty` to assist your development workflow and use `just run` as a
wrapper to `uv run` which preserves the current build configuration.

## License

<sup>
This library is licensed under the terms of the <a href="LICENSE">ISC License</a>.
</sup>
