#@echo off
pdflatex Master_Thesis
pdflatex Master_Thesis
bibtex bib
bibtex ref
makeindex Master_Thesis
makeindex Master_Thesis.glo -s Master_Thesis.ist -t Master_Thesis.glg -o Master_Thesis.gls
makeindex -s Master_Thesis.ist -t Master_Thesis.alg -o Master_Thesis.acn Master_Thesis.acr
makeindex -s Master_Thesis.ist -t Master_Thesis.slg -o Master_Thesis.syi Master_Thesis.syg
pdflatex Master_Thesis
pdflatex Master_Thesis
pdflatex Master_Thesis
pdflatex Master_Thesis
echo FERTIG!!! Taste bitte..
pause
