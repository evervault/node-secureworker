#!/bin/bash -e

# Unless --no-autoexec is specified, the first script is also an autoexec script whhich runs automatically, before
# the wanted script is run. The default autoexec script populates the Duktape environment with SecureWorker symbol
# and other global symbols.

NO_AUTOEXEC=''
FILES=()

for arg in "$@"; do
  case "$arg" in
    --no-autoexec)
      NO_AUTOEXEC='true'
    ;;
    *)
      FILES+=("$arg")
    ;;
  esac
done

if [ ${#FILES[@]} -eq 0 ]; then
  echo "No scripts provided."
  exit 1
fi

cat <<EOF
#include "scripts.h"
EOF

for file in "${FILES[@]}"; do
  base_file=$(basename "$file")
  cleaned_file=$(echo -n "$base_file" | tr -c '[A-Za-z0-9]' '_')
  echo "extern const char _binary_${cleaned_file}_start[], _binary_${cleaned_file}_end[];"
done

echo "const size_t SCRIPTS_LENGTH = ${#FILES[@]};"

echo "const duk_enclave_script_t SCRIPTS[${#FILES[@]}] = {"
for file in "${FILES[@]}"; do
  base_file=$(basename "$file")
  cleaned_file=$(echo -n "$base_file" | tr -c '[A-Za-z0-9]' '_')
  echo "  {\"${base_file}\", _binary_${cleaned_file}_start, _binary_${cleaned_file}_end},"
done
echo "};"

if [ "$NO_AUTOEXEC" = "true" ]; then
  echo "const duk_enclave_script_t *AUTOEXEC_SCRIPT = NULL;"
else
  echo "const duk_enclave_script_t *AUTOEXEC_SCRIPT = &SCRIPTS[0];"
fi