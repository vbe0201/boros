# boros build automation
# ----------------------

set dotenv-load
set positional-arguments

DEBUG_OPTS := '\
  --config-settings=setup-args="-Dbuildtype=debug" \
  --config-settings=setup-args="-Dliburing_source=auto" \
  --config-settings=install-args="--skip-subprojects"'

DEBUG_ASAN_OPTS := DEBUG_OPTS + '\
  --config-settings=setup-args="-Db_sanitize=address,undefined"'

build_base   := ".boros-build"
build_mode   := env_var_or_default("BOROS_BUILD_MODE", "debug")
project_name := "boros"

default:
  @just --list

# Update the project environment
sync mode=build_mode *args:
  #!/usr/bin/env bash
  set -euo pipefail
  case "{{mode}}" in
    debug)      meson_args="{{DEBUG_OPTS}}" ;;
    debug-asan) meson_args="{{DEBUG_ASAN_OPTS}}" ;;
    release)    meson_args="" ;;
    *) echo "Unknown build mode: {{mode}}"; exit 1 ;;
  esac

  uv sync --reinstall-package {{project_name}} \
    --config-settings=build-dir={{build_base}}.{{mode}} \
    $meson_args \
    {{args}}

# Run a command or script
run *args:
  uv run --no-sync {{args}}

# Runs tests
test *args:
  uv run --no-sync pytest {{args}}

# Runs the linter
lint *args: (run "ruff" "check" "." args)

# Formats the project code
format *args: (run "ruff" "format" "." args)

# Runs the type checker
ty *args: (run "ty" "check" args)

# Runs all checks on the project
check: lint ty (run "ruff" "format" "--check" ".")

# Add a project dependency
add *args:
  uv add --no-sync {{args}}
  @echo "Run 'just sync <mode>' to rebuild"

# Remove a project dependency
remove *args:
  uv remove --no-sync {{args}}
  @echo "Run 'just sync <mode>' to rebuild"

# Update the project's lockfile
update *args:
  uv lock --upgrade {{args}}
  @echo "Run 'just sync <mode>' to rebuild"

# Display the dependency tree
tree *args:
  uv tree {{args}}

# Builds a distribution of the project
build *args:
  rm -rf dist/
  uv build \
    --config-settings=build-dir={{build_base}}.release \
    {{args}}

# Publishes a release to PyPI
publish *args:
  uv publish {{args}}

# Simulates a release on TestPyPI
publish-test *args: (publish "--index-url" "https://test.pypi.org/simple/" args)

# Cleans up build files
clean:
  rm -rf {{build_base}}.* dist/ *.egg-info/

# Runs tests on a debug build
debug *args: (sync "debug") (test args)

# Runs tests on a debug build with ASan enabled
asan *args: (sync "debug-asan") (test args)
