targets: apriori_serial.cpp
	g++ -std=c++11 -O3 apriori_serial.cpp -o apriori_serial
clean:
	rm apriori_serial