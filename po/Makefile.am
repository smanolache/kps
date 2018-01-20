POFILES = AUTO

EXTRA_DIST = kpowersave.pot

messages:
	LIST=`find ../src -name \*.h -o -name \*.H -o -name \*.cpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		@XGETTEXT@ $$LIST -kI18N_NOOP -ki18n -ktr2i18n -o kpowersave.pot; \
		sed s/' ..\/src\/'/' '/ kpowersave.pot > kpowersave.pot_new; \
		mv kpowersave.pot_new kpowersave.pot; \
		sed s/' ..\/src\/'/' '/ kpowersave.pot > kpowersave.pot_new; \
		mv kpowersave.pot_new kpowersave.pot; \
	fi

messages_stat:
	for f in *.po; do echo -n $f :; msgfmt --stat $f ; done

messages_merge:
	for f in *.po; do echo -n $f :; msgmerge -U $f kpowersave.pot; done	

messages_diff:
	for f in *.po; \
		do \
		msgmerge $f kpowersave.pot > $f.tmp; \
		echo -n "diff $f to $f.diff"; \
		msgcomm --force-po -o $f.diff --less-than 2 $f $f.tmp; \
		rm $f.tmp; \
		done 
