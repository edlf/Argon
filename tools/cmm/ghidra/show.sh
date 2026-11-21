# show.sh <func-addr-8hex> <file> : print a decompiled function without local declarations
s=$(grep -n -E "===== (FUN_)?$1 " $2 | cut -d: -f1); e=$(awk -v s=$s 'NR>s && /^\/\/ callees/ {print NR; exit}' $2)
sed -n "$s,${e}p" $2 | grep -v '^\s*$' | grep -v -E '^  (undefined[0-9]*|float|int|uint|bool|char|short|float10|longlong|ulonglong|undefined8|byte|ushort|void) \**[a-zA-Z_0-9]+(\s*\[[0-9]+\])?;$'
