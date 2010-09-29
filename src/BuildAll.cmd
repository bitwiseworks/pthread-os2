kmk BUILD_TYPE=debug clean
kmk BUILD_TYPE=release clean

REM
REM Do not change debug/release order, it is needed to put INSTALL files into
REM same package.
REM 

kmk BUILD_TYPE=debug
kmk BUILD_TYPE=debug BUILD_AOUT=aout

kmk BUILD_TYPE=release
kmk BUILD_TYPE=release BUILD_AOUT=aout

kmk BUILD_TYPE=debug packing
kmk BUILD_TYPE=release packing
