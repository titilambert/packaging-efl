#!/bin/bash

# This script will generate a C file containing all the shaders used by Evas_3D

DIR=`dirname $0`

OUTPUT=${DIR}/evas_gl_3d_shaders.x

# Skip generation if there is no diff (or no git)
if ! git rev-parse 2>> /dev/null >> /dev/null ; then exit 0 ; fi
if git diff --quiet --exit-code -- "$DIR"
then
  touch "${OUTPUT}"
  exit 0
fi

exec 1<&-
exec 1>${OUTPUT}

SHADERS="$@"
vert_shaders_source=""
frag_shaders_source=""

# Write header
printf "/* DO NOT MODIFY THIS FILE AS IT IS AUTO-GENERATED */\n\n"
for shd in ${SHADERS} ; do
  lname=`basename ${shd} .shd`

  if echo ${lname} |grep _vert 2>&1 >> /dev/null ; then
    vert_shaders_source="${vert_shaders_source}    ${lname}_glsl,\n"
  fi
  if echo ${lname} |grep _frag 2>&1 >> /dev/null ; then
    frag_shaders_source="${frag_shaders_source}    ${lname}_glsl,\n"
  fi

  m4 ${DIR}/include.shd ${shd} > ${shd}.tmp

  OIFS=$IFS
  IFS=$'\n'
  printf "static const char ${lname}_glsl[] ="
  for line in `cat ${shd}.tmp` ; do
      printf "\n   \"${line}\\\n\""
  done
  printf ";\n\n"
  IFS=${OIFS}

  rm ${shd}.tmp
done

printf "static const char *vertex_shaders[] =
{\n"
      printf "${vert_shaders_source}"
printf "};\n\n"

printf "static const char *fragment_shaders[] =
{\n"
      printf "${frag_shaders_source}"
printf "};\n"
