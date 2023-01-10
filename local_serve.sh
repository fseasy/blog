#!/bin/sh

if [ "$1" = "full" ]; then
    inc_args=""
else
    inc_args="--incremental"
fi

if [ ! -e "assets/bef" ] && [ -e "../blog-extra-file" ]; then
    echo "Link blog-extra-file => assets/bef"
    cd assets 
    ln -s ../../blog-extra-file bef
    cd ..
fi

echo "PLEASE clone blog-extra-file to root and rename to bef"
bundle exec jekyll serve -w "$inc_args" --host 127.0.0.1
