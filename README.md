# build-time-parser



Parses simple log files produced by using the following scripts in your build phases

Before build:
```
echo "STARTED $(date +%s)" >> log.buildlog
```
After build:
```
echo "FINISHED $(date +%s)" >> log.buildlog
```


usage:

```
Usage: build-time-parser [-verbose] <filename> <filename> <...>

Example: build-time-parser joh.log robert.log erika.log
Example: build-time-parser -verbose joh.log robert.log erika.log
```
