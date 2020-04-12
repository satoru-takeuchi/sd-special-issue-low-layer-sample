a.out: main.cc
	docker run --rm -it -v $(CURDIR):$(CURDIR) -w $(CURDIR) livadk/toshokan_build_hakase:v0.10 g++ --std=c++14 -static main.cc
	docker run --rm -it -v $(CURDIR):$(CURDIR) -w $(CURDIR) livadk/toshokan_build_hakase:v0.10 strip --strip-debug a.out
	docker run --rm -it -v $(CURDIR):$(CURDIR) -w $(CURDIR) livadk/toshokan_build_hakase:v0.10 ./a.out

clean:
	-rm -f a.out
