paper.pdf:
	C:\Users\p41143\AppData\Local\Pandoc\pandoc.exe --filter pandoc-citeproc -f markdown+yaml_metadata_block+citations -s paper.md -o paper.pdf
	
testbib.pdf:
	pdflatex testbib.tex
	bibtex paper.bib
	pdflatex testbib.tex