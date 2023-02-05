find . -name "*memfault*" | sort | while read line;
do mv "$line" "${line//memfault/ticos}";
done

#find . -type f -name "*ticos*" -exec sh -c 'for f; do mv "$f" "${f//ticos/ticos}"; done' _ {} +

#find . -name "*abc*" -exec bash -c 'mv "$1" "${1/abc/def}"' - {} \;