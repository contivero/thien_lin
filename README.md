Simple implementation of a secret image sharing scheme, as discribed in
[Thien, C.C., Lin, J.C., 2002. Secret image sharing. Comput. Graphics 26 (1), 765–770]

The program hides 8-bit BMP images inside others. The 40 byte BITMAPINFOHEADER
format is assumed.

To build simply use `make`, the different flags can be found in `config.mk`

usage:

```
bmpsss (-d|-r) --secret <image> -k <number> -w <width> -h <height> [-s <seed>] [-n <number>] [--dir <directory>]

-d                  distribute image by hiding it on others
-r                  recover image hidden in others
--secret <image>     if -d was specified, image is the file name of the BMP file
                    to hide. Otherwise (if -r was specified), output file name
                    with the revealed  image.
-w <width>          width of the image to recover
-h <height>         height of the image to recover
-s <seed>           seed for the permutation. If non specified, uses 691.
-n <number>         amount of files in which to distribute the image. If not
                    specified, uses the total amount of files in the directory
--dir <directory>    directory in which to search for the images. If not
                    specified, use the current directory.
```
For some examples, see the `test_files` folder, and `script.sh`.
Note that the permutation step is coded, but currently commented out.
