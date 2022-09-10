#!/bin/sh

if [ $# -ne 1 ]
then
    echo "Usage: spec_gen.sh <root_project_folder>"
    exit
fi

if ! command -v wast2json &> /dev/null
then
    echo "wast2json could not be found, make sure it's in your PATH"
    exit
fi

spec_test_dir="$(realpath $1)/test/testsuite"

if [ ! -d $spec_test_dir ]
then
    echo "$spec_test_dir does not exist!"
    exit
fi

mkdir -p spectest
cd spectest

for t in $spec_test_dir/*.wast
do
    wast2json $t
done

cd ..