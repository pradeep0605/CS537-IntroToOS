find xv6 -name *.S > tag_files
find xv6 -name *.c >> tag_files
find xv6 -name *.h >> tag_files
ctags `cat tag_files`

