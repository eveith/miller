all: c doc
c: .always
	make -C c install
doc: .always
	cd doc && poki
.always:
	@true