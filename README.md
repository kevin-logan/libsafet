# libsafet
C++ Safe Template Library

# Building libsafet
This requires installing Catch2 inorder to build. There are a number of package managers that contain catch2. If however, your package manager does not contain Catch2 please see the following steps as this requires version 2 not the most recent version 3 or the older version 1.9 which many linux distrobutions default to when installing via their package managers.

1. git clone git@github.com:catchorg/Catch2.git
2. git checkout v2.13.9
3. cmake -Bbuild -H. -DBUILD_TESTING=OFF
4. sudo cmake --build build/ --target install

Now libsafet should build on your system

1. git clone libsafet repo
2. cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
3. cmake --build build/

