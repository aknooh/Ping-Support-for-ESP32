#!/bin/bash
#
# Unless required by applicable law or agreed to in writing, this
# software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.

PATCH_DIR=$(pwd)/patch_files/lib_changes.diff
echo "Patch $PATCH_DIR"

(cd ../../.. && git apply --reject $PATCH_DIR)
cd ../ && mv Ping/ ping/ && cd ping/

