Single photon propagating through homogenous electron background.

Run:
```bash
$ bin/picongpu --versionOnce -d 1 1 1 -g 64 64 32 -s 183 --periodic 1 1 1
```

Implement in-kernel outputs by applying
```c++
printf("New momentum (%g, %g, %g)\n", new_mom_x, new_mom_y, mom.z());
```
as the last line of the kernel lambda function.
