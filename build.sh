#!/bin/bash

SRCS="lib/glad/src/glad.c main.c"

gcc $SRCS -std=c99 -Wall -Ilib/glad/include -lSDL2 -lGL -ldl -lm -o learnopengl

