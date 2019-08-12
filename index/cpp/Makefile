PROTOC=~/third_part/bin/protoc

FLAG=-std=c++11 -g -I ~/third_part/include -L ~/third_part/lib -lprotobuf\
		 -lgflags -lglog -lpthread

.PHONY:all
all:index_dump index_builder

index_builder:index_builder.cc libindex.a
	g++ $^ -o $@ $(FLAG)
	cp -f $@ ../bin/

index_dump:index_dump.cc libindex.a
	g++ $^ -o $@ $(FLAG)
	cp -f $@ ../bin/

libindex.a:index.pb.cc index.cc
	g++ -c index.pb.cc -o index.pb.o $(FLAG)
	g++ -c index.cc -o index.o $(FLAG)
	ar -rc libindex.a index.o index.pb.o

index.pb.cc:index.proto
	$(PROTOC) index.proto --cpp_out=.

.PHONY:clean
clean:
	rm index_dump index_builder *.o libindex.a index.pb.*
