#!/bin/bash


if ! (which readlink | grep -q gnu)
then
    echo "Install GNU utils"
    echo "brew install coreutils ed findutils gawk gnu-sed gnu-tar grep make"
    echo 'export PATH="$(echo /usr/local/opt/*/libexec/gnubin | tr ' ' :):$PATH";'
    exit 1
fi

ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
ACTION="${1:-build}"

set -ex

case "$ACTION" in
    deps)
        brew list cmake || brew install cmake
        brew list qt@5 || brew install qt@5
        brew upgrade python@3.9 || true
        brew list potrace || brew install potrace
        brew list ffmpeg || brew install ffmpeg
        brew list libarchive || brew install libarchive
        ;;

    configure)
        SUFFIX="$2"
        mkdir -p "$ROOT/build"
        cd "$ROOT/build"
        cmake .. \
            -DQt5_DIR="$(brew --prefix qt@5)/lib/cmake/Qt5" \
            -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)/lib/cmake/Qt5Designer" \
            -DLibArchive_INCLUDE_DIR="$(brew --prefix libarchive)/include" \
            -DVERSION_SUFFIX="$SUFFIX"
        ;;

    build)
        cd "$ROOT/build"
        JOBS="${2:-4}"
        make -j$JOBS
        make translations || make help | sort # Why does this not work on travis?
        mkdir -p glaxnimate.iconset
        cp ../data/images/glaxnimate.png glaxnimate.iconset/icon_512x512.png
        iconutil -c icns glaxnimate.iconset -o glaxnimate.icns

        if [ -n "$TRAVIS_BRANCH" ]
        then
            function travis_is_a_pain()
            {
                while :
                do
                    echo pain
                    sleep 300
                done
            }
            travis_is_a_pain &
            cpack -G Bundle
            kill %%
        else
            cpack -G Bundle
        fi

        mv Glaxnimate-*.dmg glaxnimate.dmg
        shasum -a 1 glaxnimate.dmg >checksum.txt
        ;;

    deploy)
        BRANCH="${2:-master}"
        SSH_ARGS="$3"

        if [ "$BRANCH" = macbuild -o "$BRANCH" = master -o "$BRANCH" = github ]
        then
            path=master
            channel="mac-beta"
        else
            path="$BRANCH"
            channel="mac-stable"
        fi

        cd "$ROOT/build"
        mkdir -p "artifacts/$path/MacOs"
        cp glaxnimate.dmg "artifacts/$path/MacOs"
        cp checksum.txt "artifacts/$path/MacOs"
        cd artifacts

        # Upload SourceForge
        rsync -a "$path" mbasaglia@frs.sourceforge.net:/home/frs/project/glaxnimate/ -e "ssh -o StrictHostKeyChecking=no $SSH_ARGS"
        ;;

    pypi)
        cd "$ROOT/build"
        make glaxnimate_python_depends_install
        make glaxnimate_python
        libpath="$(echo 'from distutils.util import get_platform; import sys; print("lib.%s-%d.%d" % (get_platform(), *sys.version_info[:2]))' | python3)"
        ln -sf lib bin/python/build/$libpath
        make glaxnimate_python_wheel
        ;;

    *)
        echo " # Install dependencies"
        echo "mac_build.sh deps"
        echo
        echo " # Configure CMake"
        echo "mac_build.sh configure [VERSION_SUFFIX]"
        echo
        echo " # Compile / package"
        echo "mac_build.sh build [JOBS=4]"
        echo
        echo " # Add package to artifacts"
        echo "mac_build.sh deploy [BRANCH=master [SSH_ARGS]]"
        echo
        echo " # Build python package"
        echo "mac_build.sh pypi"
        ;;
esac
