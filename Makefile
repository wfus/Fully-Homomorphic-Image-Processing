define cmake_build_target
	mkdir -p build
	cd build && cmake .. && make && cd ..
endef

all:
	$(call cmake_build_target, all)

clean:
	rm -rf bin/*


