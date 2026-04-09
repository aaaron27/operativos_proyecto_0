all: compile run clean

compile: main.c
	gcc main.c -o main -lm

run: compile
	./main

test_cases: compile
	./main < test_cases/carnage/case1.txt
	./main < test_cases/carnage/case2.txt
	./main < test_cases/traffic_lights/case1.txt
	./main < test_cases/traffic_lights/case2.txt
	./main < test_cases/traffic_officers/case1.txt
	./main < test_cases/traffic_officers/case2.txt

clean:
	rm -f main
