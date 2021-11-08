.PHONY: release debug clean tests run

debug:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=DEBUG ..
	cd build; make -j4

release:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=RELEASE ..
	cd build; make -j4

clean:
	rm -rf build

tests:
	cd tests; python3 gen_scenario.py; python3 test.py; cd..

run:
	mpirun -np 20 --hostfile hostfile ./build/algorep --servers 17 --clients 2

%:
	@:
