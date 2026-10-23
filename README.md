# Nightship demo app

A tiny Windows app that exists to be delivered by [Nightship](https://nightship.io).

It shows one line of text, very large: the contents of `message.txt` next to
the exe (`Hello Demo` out of the box). Change the text, publish a new build,
press **Run** in the Nightship client - and the new text is what you see.
A small line underneath names the build that is running.

```
nightship-demo.exe                    text from message.txt
nightship-demo.exe --text "QA Mode"   this text instead
```

The `--text` argument is what makes several Run buttons on one channel
useful: the same build, started different ways (launch targets).

## Build

Windows, Visual Studio 2022 (or its Build Tools) and CMake:

```
cmake -S . -B build
cmake --build build --config Release
build/Release/nightship-demo.exe
```

Plain Win32 with a static C runtime - the exe needs nothing next to it
except `message.txt`. `Esc` closes the window.
