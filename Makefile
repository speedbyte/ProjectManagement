#
#  $Id: Makefile,v 1.2 2005/07/05 08:03:54 hartko Exp $
#

#
#  Multi-Sensor Data Fusion Framework Study
#
#  Makefile for LaTeX documentation
#
SHELL=/bin/bash
TOP=$(shell pwd)

JSOB=$(TOP)/helikopter-raspberry
HELIKOPTER-RASPBERRY=$(TOP)/helikopter-raspberry/user_manual_latex
QUAD=$(TOP)/QuadrocopterMigration
all:
	# try
	cd $(HELIKOPTER-RASPBERRY) && pdflatex user_manual
	read
	cd $(QUAD) && pdflatex QuadrocopterMigration
clean:
	rm -f *.log *.dvi *.aux *.toc *.fig.bak *~
