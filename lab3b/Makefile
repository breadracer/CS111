default: lab3b

lab3b:
	@touch lab3b
	@printf 'python3 main.py $$1' >> lab3b
	@chmod +x lab3b

dist: lab3b
	@tar -czf lab3b-204996814.tar.gz main.py Makefile README

clean:
	@rm -f lab3b *.tar.gz
