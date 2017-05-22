rem #@echo off
pdflatex Interim_Report
pdflatex Interim_Report
bibtex bib
bibtex ref
makeindex Interim_Report
makeindex Interim_Report.glo -s Interim_Report.ist -t Interim_Report.glg -o Interim_Report.gls
makeindex -s Interim_Report.ist -t Interim_Report.alg -o Interim_Report.acn Interim_Report.acr
makeindex -s Interim_Report.ist -t Interim_Report.slg -o Interim_Report.syi Interim_Report.syg
pdflatex Interim_Report
pdflatex Interim_Report
pdflatex Interim_Report
pdflatex Interim_Report
echo FERTIG!!! Taste bitte..
pause
