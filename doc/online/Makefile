all:
	meinproc --stylesheet /opt/kde3/share/apps/ksgmltools2/customization/kde-web.xsl ../en/index.docbook
	./replace
	rm *.bak
	cp ../en/*.png .

clean:
	rm *.png *.html
