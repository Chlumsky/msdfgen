#!/bin/bash

source ./platform_detect.sh

cd ../bin
# Create build subdirectories
mkdir minimal 2> /dev/null | true
mkdir minimal32 2> /dev/null | true
mkdir openmp 2> /dev/null | true
mkdir openmp32 2> /dev/null | true
mkdir complete 2> /dev/null | true

version=$(git describe --tags) # Get version name from current git tag
executable_suffix=""


use_32_bit=false # Per default do not build 32 bit

case $platform_suffix in
    "win")
        use_32_bit=true # Build 32 bit as well for windows
        executable_suffix=".exe" # windows has .exe as executable suffix
        ;;
esac

Build() {
    Arch="$1"
    Configuration="$2"
    
    OUTPUT_NAME="msdf-${Configuration}-${version}-${platform_suffix}-${Arch}"
    BUILD_DIR_PREFIX="."
    
    Generator="Unix Makefiles" # Default to unix makefiles
    case $platform_suffix in
        "win") 
            case $Arch in
                "x64") GeneratorArch="-A x64" ;;
                "x86") GeneratorArch="-A Win32" ;;
            esac
            BUILD_DIR_PREFIX="Release" # VS builds put output files in a $Configuration sub directory
            Generator="Visual Studio 16"
            ;;
        "linux")
            case $Arch in
                "x64") GeneratorArch="-m 64" ;;
                "x86") GeneratorArch="-m 32" ;;
            esac
            ;;
    esac
    case $Configuration in
        "minimal") PARAMS="" ;; # No additional settings
        "openmp") PARAMS="-DMSDFGEN_USE_OPENMP=ON" ;; # Enable openmp in build
    esac
    # Conigure build
    cmake -DCMAKE_BUILD_TYPE=Release -G "$Generator" $GeneratorArch ../.. -DMSDFGEN_USE_CPP11=ON -DMSDFGEN_BUILD_MSDFGEN_STANDALONE=ON $PARAMS || return $?
    # Build
    cmake --build . --config Release -j 4 || return $?
    # Create collected output directory for easy archiving
    mkdir ../complete/$OUTPUT_NAME/ 2> /dev/null | true
    cp $BUILD_DIR_PREFIX/msdfgen$executable_suffix ../complete/$OUTPUT_NAME/
    case $platform_suffix in
    "win")
        # Archive with powershell into zip
        powershell "Compress-Archive ../complete/$OUTPUT_NAME/* ../complete/$OUTPUT_NAME.zip" || return $?
        ;;
    *) # Default to tar.gz
        pushd ../complete/$OUTPUT_NAME
        tar -zcvf ../$OUTPUT_NAME.tar.gz * || return $?
        popd
        ;;
    esac
}

#Minimal builds

cd minimal
Build "x64" "minimal" || exit $?
cd ..

if [ "$use_32_bit" = true ] ; then
    cd minimal32
    Build "x86" "minimal" || exit $?
    cd ..
fi

# OpenMP builds

cd openmp
Build "x64" "openmp" || exit $?
cd ..

if [ "$use_32_bit" = true ] ; then
    cd openmp32
    Build "x86" "openmp" || exit $?
    cd ..
fi