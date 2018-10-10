BIN = ./node_modules/.bin
MOCHA_OPTS = --timeout 10s --recursive
REPORTER = spec
TEST_FILES = test/test.js

all:
	node-gyp configure build

clean:
	rm -rf build

