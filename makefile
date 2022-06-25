gbn:  
	gcc -o go-back-n.out go-back-n.c
	./go-back-n.out

abp:
	gcc -o alternating-bit-protocol.out alternating-bit-protocol.c
	./alternating-bit-protocol.out

clean:
	rm -f *.out