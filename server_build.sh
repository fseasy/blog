#!/bin/sh

cp Gemfile.US Gemfile
bundle update -V
bundle exec jekyll build
