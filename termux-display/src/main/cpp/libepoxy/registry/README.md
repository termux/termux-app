## Updating the registry XML

In order to update the registry XML files and retain the history you cannot
simply download the files the [Khronos website](https://khronos.org/registry/OpenGL/index_gl.php)
and copy them into this directory. You should follow these steps, instead:

 1. check out the `khronos-registry` branch
 2. download the XML files from the Khronos repository
 3. copy them under the `registry` directory
 4. check the result for consistency and commit it
 5. check out the `master` branch and merge the `khronos-registry` branch
    into it with the appropriate commit message

