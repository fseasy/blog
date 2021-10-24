#!/bin/sh

cp Gemfile.US Gemfile
bundle update
bundle exec jekyll build
