# COPR "Custom/make srpm" source method: copr runs  make -f .copr/Makefile srpm outdir=<dir> spec=<spec>
# from a checkout of this repository and builds the resulting source RPM for every Fedora release.
spec ?= packaging/rpm/lumina.spec
outdir ?= .

srpm:
	dnf -y install git rpm-build >/dev/null 2>&1 || true
	ver=$$(awk '/^Version:/{print $$2}' $(spec)); \
	git archive --prefix=lumina-player-$$ver/ -o $(outdir)/lumina-player-$$ver.tar.gz HEAD; \
	rpmbuild -bs $(spec) --define "_sourcedir $(outdir)" --define "_srcrpmdir $(outdir)"
