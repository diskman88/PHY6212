#!/bin/sh

L_PATH=`pwd`
TOOLCHAIN=`which csky-abiv2-elf-gdb`
ROOT_DIR=$1

VSC_LAUNCH=${ROOT_DIR}.vscode/launch.json
mkdir -p ${ROOT_DIR}.vscode

echo '{' > ${VSC_LAUNCH}
echo '    "version": "0.2.0",' >> ${VSC_LAUNCH}
echo '    "configurations": [' >> ${VSC_LAUNCH}
echo '    {' >> ${VSC_LAUNCH}
echo '        "name": "(gdb) Bash on Windows Launch",' >> ${VSC_LAUNCH}
echo '        "type": "cppdbg",' >> ${VSC_LAUNCH}
echo '        "request": "launch",' >> ${VSC_LAUNCH}
echo '        "program": "yoc.elf",' >> ${VSC_LAUNCH}
echo '        "args": [],' >> ${VSC_LAUNCH}
echo '        "stopAtEntry": false,' >> ${VSC_LAUNCH}
echo "        \"cwd\": \"${L_PATH}\"," >> ${VSC_LAUNCH}
echo '        "environment": [],' >> ${VSC_LAUNCH}
echo '        "externalConsole": true,' >> ${VSC_LAUNCH}
echo '        "pipeTransport": {' >> ${VSC_LAUNCH}
echo "            \"debuggerPath\": \"${TOOLCHAIN}\"," >> ${VSC_LAUNCH}
echo '            "pipeProgram": "C:\\\\Windows\\\\sysnative\\\\bash.exe",' >> ${VSC_LAUNCH}
echo '            "pipeArgs": ["-c"],' >> ${VSC_LAUNCH}

echo -n "            \"pipeCwd\": " >> ${VSC_LAUNCH}
echo "\"${L_PATH}\"" | awk 'gsub("/mnt/","",$1)' | awk 'gsub("/","\\\\",$1)' | awk 'sub("\\\\",":\\",$1)' >> ${VSC_LAUNCH}

echo '        },' >> ${VSC_LAUNCH}
echo '        "sourceFileMap": {' >> ${VSC_LAUNCH}

echo ${L_PATH} | awk '{printf "            \"%s\": \"%s:\\\\\"\n",substr($0,1,7),substr($0,6,1)}' >> ${VSC_LAUNCH}

echo '        },' >> ${VSC_LAUNCH}
echo '        "setupCommands": [' >> ${VSC_LAUNCH}
echo '            {' >> ${VSC_LAUNCH}
echo '                "description": "Enable pretty-printing for gdb",' >> ${VSC_LAUNCH}
echo '                "text": "-enable-pretty-printing",' >> ${VSC_LAUNCH}
echo '                "ignoreFailures": true' >> ${VSC_LAUNCH}
echo '            }' >> ${VSC_LAUNCH}
echo '        ]' >> ${VSC_LAUNCH}
echo '    }' >> ${VSC_LAUNCH}
echo '    ]' >> ${VSC_LAUNCH}
echo '}' >> ${VSC_LAUNCH}




