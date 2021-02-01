#!/bin/bash
# struct fileItem {
#         char *fileName;
#         uint32_t fileSize;
#         uint8_t *fileData;
# };

GZIP="gzip"
DIR=web

if [ -n "$1" ]; then
	DIR=$1
fi

TDIR=$(mktemp -d)

files="
style.css
login.html
jquery.dform-1.1.0.min.js
jquery-3.5.1.min.js
js.cookie-2.2.1.min.js
login.js
sha1.min.js
fail.png
check.png
logo.png
"

files=$(echo "$files" | sort)

for f in $files; do
	cp $DIR/$f $TDIR

	case "$f" in
	*.png | *.json )
        ;;
	*)
        $GZIP $TDIR/$f
        mv "$TDIR/$f.gz" $TDIR/$f
        ;;
    esac

done

printf '#include <stdint.h>\n\n'
echo 'const struct fileItem fileSystem[] = {'


for f in $files; do
	echo '        {'
	printf '                \"/%s\",\n' $f
	printf '                %d,\n' $(cat ${TDIR}/$f | wc -c)
	echo '        },'
done

printf '        {"",0}\n};\n\n'

echo 'const uint8_t fsData[] = {'
for f in $files; do
	echo "  // $f"
	cat ${TDIR}/$f | xxd -i | head -c -1
	printf ',\n\n'
done
echo '};'

rm -rf $TDIR
