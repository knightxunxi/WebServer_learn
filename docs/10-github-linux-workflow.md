# GitHub and Linux Workflow

This project is developed on the local workspace and should be easy to clone and run on a Linux virtual machine.

## First Push to GitHub

Create an empty repository on GitHub, then run:

```bash
git remote add origin git@github.com:<user>/<repo>.git
git push -u origin main
```

If HTTPS is preferred:

```bash
git remote add origin https://github.com/<user>/<repo>.git
git push -u origin main
```

## Clone on Linux

```bash
git clone git@github.com:<user>/<repo>.git
cd <repo>
```

## Install Basic Dependencies

Ubuntu example:

```bash
sudo apt update
sudo apt install -y build-essential cmake git
```

Optional tools:

```bash
sudo apt install -y gdb valgrind wrk apache2-utils linux-tools-common
```

## Build and Test

```bash
bash scripts/build.sh
bash scripts/test.sh
```

## Suggested Branch Workflow

```text
main
  stable learning milestones

feature/<module-name>
  one module or one milestone

docs/<topic>
  documentation-only updates
```

Suggested commit style:

```text
docs: initialize stage 1 roadmap
build: add cmake baseline
net: add event loop
test: add buffer tests
bench: record webserver wrk result
```

