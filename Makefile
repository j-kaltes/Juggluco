
.PHONY:
mkgit.sh:  
	find   -L -files0-from dirs.0 -maxdepth 1  -type f  -printf 'git add %p\n' > mkgit.sh
