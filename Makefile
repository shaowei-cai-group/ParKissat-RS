all:
	##################################################
	###               kissat_mab                   ###
	##################################################
	chmod a+x kissat_mab/configure kissat_mab/scripts/*.sh
	cd kissat_mab && ./configure --compact
	+ $(MAKE) -C kissat_mab

	##################################################
	###                 PaInleSS                   ###
	##################################################
	+ $(MAKE) -C painless-src
	mv painless-src/painless parkissat

clean:
	##################################################
	###                 PaInleSS                   ###
	##################################################
	+ $(MAKE) clean -C painless-src
	+ $(MAKE) clean -C kissat_mab
	rm -f parkissat
