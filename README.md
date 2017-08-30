# LoadLibrary
A simple dll injector for windows based on WINAPI's LoadLibrary function.
The Inject project compiles a simple dll that pops up a messagebox on successful injection.

# Usage
Configure the *config.ini* file, stored in the same directory as the program, with the name of the executable to inject into and the name of the dll to be injected like so:
```
[Options]
process="calc.exe"
dll="inject.dll"
```
