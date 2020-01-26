# Contributing to the Turpial Radio Firmware.

Thanks for taking the time to contribute!

The following is a set of guidelines for contributing to [**Locha**](), [**Turpial**]() or .

## Table of contents

[Contributing to the Turpial Radio Firmware.](#contributing-to-the-turpial-radio-firmware)

[Before starting](#before-starting)

* [How can I contribute?](#how-can-i-contribute)
  - [I have a question.](#i-have-a-question)
  - [Suggesting features and reporting bugs.](#suggesting-features-and-reporting-bugs)
  - [Making a Pull-Request.](#making-a-pull-request)
  - [Making a change.](#making-a-change)

[Styleguide](#styleguide)

## Before starting.

Please read our [code of conduct.](CODE_OF_CONDUCT.md)

## How can I contribute?

We'll pleased to accept your patches and contributions to this project. There
are some guidelines you need to follow.

### I have a question.

For any question you can send us a message via Twitter [@Locha_io](https://twitter.com/Locha_io), our Telegram group [t.me/Locha_io](https://t.me/Locha_io), or through our website
[locha.io](https://locha.io)

### Suggesting features and reporting bugs.

You can use our issue tracker to share your idea, it will be discussed by the
Locha Mesh team members. If we all agree and makes sense to implement this
feature, it will be kept opened.

You can open a new issue reporting a bug in our repository, please provide
detailed information about the issues you're expecting.

### Making a Pull-Request.

You can contribute making a Pull-Request with the code you want to fix, or with
the features that you would like to implement. It will follow a review process
first, and after that it can be merged.

### Making a change.

Antes de empezar a hacer modificaciones ejecuta estos comandos para crear una nueva rama que esté sincronizada con dev:

```bash
git fetch --all

git checkout dev

# Synchronize with the origin branch.
git pull origin dev

# Create a branch for your feature.
git checkout -b featurebranch

# Push your changes to your fork, assuming pr is your remote.
git push pr featurebranch
```

## Styleguide

Actually we use the same style Contiki-NG uses for it's code, you can more
information about it
[here](https://github.com/contiki-ng/contiki-ng/wiki/Code-style).
