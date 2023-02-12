#!/bin/sh

if [ "$1" = "full" ]; then
    inc_args=""
else
    inc_args="--incremental"
fi

echo "PLEASE clone blog-extra-file to root and rename to bef"
bundle exec jekyll serve -w "$inc_args" --host 0.0.0.0
