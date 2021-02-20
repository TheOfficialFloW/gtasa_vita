all:
	vita-libs-gen nids.yml out
	$(MAKE) -C out

clean:
	$(RM) -r out

install: all
	@mkdir -p $(VITASDK)/arm-vita-eabi/lib/
	cp out/libSceLibcBridge_stub.a $(VITASDK)/arm-vita-eabi/lib/
	cp out/libSceLibcBridge_stub_weak.a $(VITASDK)/arm-vita-eabi/lib/
