#! /usr/bin/env bash

find . -not -path "./.git/*" -type f | xargs du -h | sort -h
