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

cat <<EOF
#include "scripts.h"
#ifdef _WIN32
#define BINARY(NAME) binary_ ## NAME
#else
#define BINARY(NAME) _binary_ ## NAME
#endif
EOF

for file in "${FILES[@]}"; do
  cleaned_file=$(echo -n "$file" | tr -c '[A-Za-z0-9]' '_')
  echo "extern const char BINARY(${cleaned_file}_start)[], BINARY(${cleaned_file}_end)[];"
done

echo "const size_t SCRIPTS_LENGTH = ${#FILES[@]};"

echo "const duk_enclave_script_t SCRIPTS[${#FILES[@]}] = {"
for file in "${FILES[@]}"; do
  base_file=$(basename "$file")
  cleaned_file=$(echo -n "$file" | tr -c '[A-Za-z0-9]' '_')
  echo "  {\"${base_file}\", BINARY(${cleaned_file}_start), BINARY(${cleaned_file}_end)},"
done
echo "};"

if [ "$NO_AUTOEXEC" = "true" ]; then
  echo "const duk_enclave_script_t *AUTOEXEC_SCRIPT = NULL;"
else
  echo "const duk_enclave_script_t *AUTOEXEC_SCRIPT = &SCRIPTS[0];"
fi