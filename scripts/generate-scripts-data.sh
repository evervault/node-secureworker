#!/bin/bash -e

if [ "$#" -eq 0 ]; then
  echo "No scripts provided."
  exit 1
fi

cat <<EOF
.section scripts, "a"
EOF

for file in "$@"; do
  base_file=$(basename "$file")
  cleaned_file=$(echo -n "$base_file" | tr -c '[A-Za-z0-9]' '_')

  cat <<-EOF

		.global _binary_${cleaned_file}_start
		.type _binary_${cleaned_file}_start, @object
		.align 4

		_binary_${cleaned_file}_start:
		.incbin "$file"

		.global _binary_${cleaned_file}_end
		.type _binary_${cleaned_file}_end, @object

		_binary_${cleaned_file}_end:

		.global _binary_${cleaned_file}_size
		.align 4
		_binary_${cleaned_file}_size = _binary_${cleaned_file}_end - _binary_${cleaned_file}_start
	EOF
done
