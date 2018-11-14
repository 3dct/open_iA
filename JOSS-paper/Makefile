paper.pdf:
	C:\Users\p41143\AppData\Local\Pandoc\pandoc.exe --verbose --filter pandoc-citeproc -f markdown+yaml_metadata_block -s paper.md -o paper.pdf --csl ieee.csl --template=template.tex
	
testbib.pdf:
	pdflatex testbib.tex
	bibtex paper.bib
	pdflatex testbib.tex