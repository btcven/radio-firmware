# Contributing to the Turpial radio firmware

Thanks for taking the time to contribute!

The following is a set of guidelines for contributing to [**Locha**](), [**Turpial**]() or .

#### Table of contents

* [Before starting](#before-starting)

* [How can i contribute?](#how-can-i-contribute)
    * [I just have a question](#i-just-have-a-question)
    * [Suggesting **enhancements**](#suggesting-enhancements)
    * [Reporting **bugs**](#reporting-bugs)
    * [Pull request](#pull-request)

- [Contributing to the Turpial radio firmware](#contributing-to-the-turpial-radio-firmware)
      - [Table of contents](#table-of-contents)
  - [Before starting](#before-starting)
  - [How can I contribute?](#how-can-i-contribute)
    - [I have a question.](#i-have-a-question)
    - [Suggesting features and reporting bugs.](#suggesting-features-and-reporting-bugs)
    - [Making a Pull-Request.](#making-a-pull-request)
    - [Making a change.](#making-a-change)
  - [Styleguide](#styleguide)
    - [Commit messages](#commit-messages)
    - [Code styleguide](#code-styleguide)
      - [Include guards](#include-guards)
      - [Include statements](#include-statements)
      - [Naming rules](#naming-rules)
    - [Common statements](#common-statements)
      - [if / if-else](#if--if-else)
      - [while / do-while loops](#while--do-while-loops)
    - [Indentation](#indentation)
    - [Documentation styleguide](#documentation-styleguide)

## Before starting
Please read our [code of conduct](CODE_OF_CONDUCT.md)

## How can I contribute?
We'll pleased to accept your patches and contributions to this project. There 
are some guidelines you need to follow.

### I have a question.
If you have a question you can write to use using Twitter at
[@Locha_io](https://twitter.com/Locha_io), or through our website
[locha.io](https://locha.io).

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

### Commit messages
A commit message must be short, clear and a general description of the changes or improvements.
If a commit includes changes in several files or sections, we can include after the initial message a more extended description of each change.

### Code styleguide

#### Include guards
Local header files must contain an distinctly named include guard to avoid problems with including the same header multiple times, for example:
```cpp
// file: foo.h
#ifndef FOO_H
#define FOO_H
...
#endif // FOO_H
```

#### Include statements

Include statements must be located at the top of the file **only**. By default this statement will go in the .cpp files, not in header files (.h), except when necessary and it should be sorted and grouped.

#### Naming rules

- Use a descriptive name and be consistent in style when write code
- All names should be written in English

**Macros** Use uppercase and underscore
```cpp

#define LOW_NIBBLE(x) (x & 0xf)
```

**Variable names** Use underscore, dont't group variables by types
```cpp
// GOOD, underscore
int rssi_level;
int snr_level;

// BAD, Grouped
int rssi_level, snr_level;

// BAD, CamelCase
int RssiLevel;
int SnrLevel;

// BAD, mixed case
int rssiLevel;
int snrLevel;
```

**Methods or functions**  Use descriptive verbs and mixed case starting with lower case.

 ```cpp
 int getTotalNearestNodes();
```

**Classes** Use CamelCase
```cpp
class SomeClass { 
    public:
        ...
    private:
        int foo_; // Private variables ends with an underscore
};
```
### Common statements

#### if / if-else

- Always put braces around the if-else statement or when is nested in another if statement
- Put space between `if` and `()`


```cpp
// if-else or nested if-else statements ALWAYS put braces
if (foo)
{
    if(foo == 1)
    {
        bar = UP;
    }
    else 
    {
        bar = DOWN;
    }

}
else 
{
    bar = MIDDLE;
}

// only if statement
if (foo)
    bar = UP;
```
#### while / do-while loops

- Put space between `while` and `()`
```cpp
// while statement
while (foo > 0)
{
    bar++;
}

// do-while statement
do
{
    bar++;
}
while (foo > 0);

```
### Indentation

- Do not use tabs
- Use 4 spaces as default identation

### Documentation styleguide
ToDo
