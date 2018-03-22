helib: ntl 
	@mkdir -p deps
	@[[ -d deps/$(HELIB) ]] || \
	(       git clone https://github.com/shaih/HElib.git deps/$(HELIB) && \
                cd deps/$(HELIB) && \
                git checkout baf0fc1e2a8cf7173a7711be5e2dd4d4d8b16f01 && \
                cp ../../helib-makefile src/Makefile \
        )
	@cd deps/$(HELIB)/src; make

ntl:
	@mkdir -p deps
	@[[ -d deps/$(NTL) ]] || \
	(   cd deps && \
                wget http://www.shoup.net/ntl/$(NTL).tar.gz -O $(NTL).tgz && \
                tar xzf $(NTL).tgz && \
                rm -f $(NTL).tgz && \
                cd $(NTL)/src && \
                ./configure WIZARD=off && \
                cd ../include/NTL \
        )
	@cd deps/$(NTL)/src; make

clean:
	rm -f aes
	rm -f multest
	rm -f simon-simd
	rm -f simon-blocks
	rm -f simon-pt
	rm -f $(BLDDIR)/*.o
	rm -f *.bc
	rm -f $(BLDDIR)/*.bc

