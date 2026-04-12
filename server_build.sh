#!/bin/sh

cp Gemfile.US Gemfile
bundle update
# Must set JEKYLL_ENV=production, so that some development plugins won't be active
JEKYLL_ENV=production bundle exec jekyll build
