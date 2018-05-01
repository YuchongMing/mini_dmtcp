# MINI-DMTCP
MINI-DMTCP is a project to sav the checkpoint image of an single process running process and restore it at any time on the same platform.

Feature of the mini-dmtcp project:

- Load from the library and tigger from the system signal SIGUSR2
- Save the process data into image file 'myckpt'
- Reload the orginal process from image file at any time

## Getting start

This instruction will get you a copy of the project up and running on your local machine for development and testing purposes.

### Installing

Download the tar.gz ball and unpack it using command
```
$tar -zxf hw2.tar.gz
```

### Running

To generate the checkpoint image file, please run
```
$make check
```

To restore the process from image file, please run
```
make res
```

To clean up the compile file, please run
```
$make clean
```

To debug the restart.c, please run
```
$make gdb
```

## Help

- If you saw the error as below while using ```make check```, please just ignore it
```
	makefile:37: recipe for target 'check' failed
	make: *** [check] Error 137
```

- If the restart program doesn't run automaticly while using ```make check```, please input```make res``` to run it.

## Author

* **Yuchong Ming** - *Jan, 30, 2018*

